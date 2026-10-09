#include "bookmgmt/Acquisition.h"

#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

void AcquisitionManager::setTaxRates(double printRate, double electronicRate) {
    if (printRate < 0.0 || electronicRate < 0.0)
        throw std::invalid_argument("tax rates must not be negative");
    printTaxRate_ = printRate;
    electronicTaxRate_ = electronicRate;
}

Money AcquisitionManager::calculatePostTax(const Resource& r, Money preTax) const {
    double rate = r.isDigital() ? electronicTaxRate_ : printTaxRate_;
    if (rate == 0.0) return preTax;
    std::int64_t taxMinor = static_cast<std::int64_t>(std::llround(preTax.minorUnits() * rate));
    return preTax + Money::fromMinor(taxMinor, preTax.currency());
}

void AcquisitionManager::setDepartmentBudget(const std::string& dept, Budget b) {
    departmentBudgets_.insert_or_assign(dept, std::move(b));
}

Budget* AcquisitionManager::departmentBudget(const std::string& dept) {
    auto it = departmentBudgets_.find(dept);
    return it == departmentBudgets_.end() ? nullptr : &it->second;
}

const Budget* AcquisitionManager::departmentBudget(const std::string& dept) const {
    auto it = departmentBudgets_.find(dept);
    return it == departmentBudgets_.end() ? nullptr : &it->second;
}

Budget& AcquisitionManager::getBudgetFor(const std::string& department) {
    if (department.empty()) return budget_;
    auto it = departmentBudgets_.find(department);
    if (it != departmentBudgets_.end()) return it->second;
    throw NotFoundError("department budget not found: " + department);
}

const Budget& AcquisitionManager::getBudgetFor(const std::string& department) const {
    if (department.empty()) return budget_;
    auto it = departmentBudgets_.find(department);
    if (it != departmentBudgets_.end()) return it->second;
    throw NotFoundError("department budget not found: " + department);
}

Money AcquisitionManager::quotePreTax(const std::string& id, int quantity) const {
    return catalog_.get(id).costFor(quantity);
}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    const Resource& r = catalog_.get(id);
    Money preTax = r.costFor(quantity);
    return calculatePostTax(r, preTax);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason,
                                     const std::string& department) const {
    std::string why;
    const Resource* r = catalog_.find(id);
    if (!r) {
        why = "resource not found: " + id;
    } else if (quantity <= 0) {
        why = "quantity must be positive";
    } else {
        try {
            const Budget& b = getBudgetFor(department);
            Money preTax = r->costFor(quantity);
            Money postTax = calculatePostTax(*r, preTax);
            why = b.check(r->category(), quantity, postTax, id);
        } catch (const std::exception& e) {
            why = e.what();
        }
    }
    if (reason) *reason = why;
    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(const Resource* r, const std::string& id,
                                           int qty, Money postTaxCost, Money preTaxCost,
                                           bool approved, std::string reason,
                                           std::string vendor, std::string dept,
                                           bool isCancel) {
    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        postTaxCost,
        approved,
        std::move(reason),
        preTaxCost,
        std::move(vendor),
        std::move(dept),
        isCancel
    });
    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity,
                                                   const std::string& department) {
    const Resource& r = catalog_.get(id);        // may throw NotFoundError
    const Money preTax = r.costFor(quantity);     // may throw invalid_argument
    const Money postTax = calculatePostTax(r, preTax);
    Budget& b = getBudgetFor(department);
    b.commit(r.category(), quantity, postTax, id);  // may throw quota/budget errors
    catalog_.addHoldings(id, quantity);
    std::string vendor = "Direct";
    auto best = r.cheapestVendor();
    if (best) vendor = best->vendor;
    return record(&r, id, quantity, postTax, preTax, true, {}, vendor, department, false);
}

const PurchaseRecord& AcquisitionManager::cancelOrder(int orderNo) {
    PurchaseRecord* target = nullptr;
    for (auto& rec : history_) {
        if (rec.orderNo == orderNo) {
            target = &rec;
            break;
        }
    }
    if (!target || !target->approved || target->isCancellation) {
        throw std::invalid_argument("order cannot be cancelled or already cancelled: #" +
                                    std::to_string(orderNo));
    }
    const Resource& r = catalog_.get(target->resourceId);
    Budget& b = getBudgetFor(target->department);
    b.refund(r.category(), target->quantity, target->cost, target->resourceId);
    catalog_.addHoldings(target->resourceId, -target->quantity);

    // Negative cost and quantity to reflect refund
    Money negPost = Money::fromMinor(-target->cost.minorUnits(), target->cost.currency());
    Money negPre = Money::fromMinor(-target->preTaxCost.minorUnits(), target->preTaxCost.currency());

    return record(&r, target->resourceId, -target->quantity, negPost, negPre, true,
                  "Cancelled order #" + std::to_string(orderNo), target->vendor,
                  target->department, true);
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs, bool allOrNothing) {

    if (allOrNothing) {
        // Simulation phase: verify if EVERY request would succeed
        // Work on temporary copies of budgets
        Budget simDefault = budget_;
        std::map<std::string, Budget> simDepts = departmentBudgets_;
        bool anyFailed = false;
        std::vector<std::string> failReasons;
        failReasons.reserve(reqs.size());

        for (const auto& req : reqs) {
            const Resource* r = catalog_.find(req.resourceId);
            std::string why;
            if (!r) {
                why = "resource not found: " + req.resourceId;
            } else if (req.quantity <= 0) {
                why = "quantity must be positive";
            } else {
                Money preTax = r->costFor(req.quantity);
                Money postTax = calculatePostTax(*r, preTax);
                Budget* b = nullptr;
                if (req.department.empty()) {
                    b = &simDefault;
                } else {
                    auto it = simDepts.find(req.department);
                    if (it != simDepts.end()) b = &it->second;
                }
                if (!b) {
                    why = "department budget not found: " + req.department;
                } else {
                    why = b->check(r->category(), req.quantity, postTax, req.resourceId);
                    if (why.empty()) {
                        b->commit(r->category(), req.quantity, postTax, req.resourceId);
                    }
                }
            }
            if (!why.empty()) anyFailed = true;
            failReasons.push_back(why);
        }

        if (anyFailed) {
            // All-or-nothing rejection: nothing bought
            std::vector<PurchaseRecord> results;
            results.reserve(reqs.size());
            for (std::size_t i = 0; i < reqs.size(); ++i) {
                const auto& req = reqs[i];
                const Resource* r = catalog_.find(req.resourceId);
                Money preTax = r ? r->costFor(req.quantity > 0 ? req.quantity : 1) : Money{};
                Money postTax = r ? calculatePostTax(*r, preTax) : Money{};
                std::string reason = failReasons[i].empty()
                    ? "batch rejected due to all-or-nothing policy"
                    : failReasons[i];
                results.push_back(record(r, req.resourceId, req.quantity, postTax, preTax,
                                         false, reason, "Direct", req.department, false));
            }
            return results;
        }
    }

    // Normal processing
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());
    for (const auto& req : reqs) {
        const Resource* r = catalog_.find(req.resourceId);
        Money preTax;
        Money postTax;
        std::string why;
        if (!r) {
            why = "resource not found: " + req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            try {
                preTax = r->costFor(req.quantity);
                postTax = calculatePostTax(*r, preTax);
                const Budget& b = getBudgetFor(req.department);
                why = b.check(r->category(), req.quantity, postTax, req.resourceId);
            } catch (const std::exception& e) {
                why = e.what();
            }
        }

        if (why.empty()) {
            results.push_back(purchase(req.resourceId, req.quantity, req.department));
        } else {
            std::string vendor = "Direct";
            if (r) {
                auto best = r->cheapestVendor();
                if (best) vendor = best->vendor;
            }
            results.push_back(record(r, req.resourceId, req.quantity, postTax, preTax,
                                     false, why, vendor, req.department, false));
        }
    }
    return results;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& rec : history_) {
        if (rec.approved) {
            sum += rec.cost;
        }
    }
    return sum;
}

Money AcquisitionManager::totalPreTaxSpent() const {
    Money sum;
    for (const auto& rec : history_) {
        if (rec.approved) {
            sum += rec.preTaxCost;
        }
    }
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";
    for (const auto& rec : history_) {
        std::string status = rec.approved
            ? (rec.isCancellation ? "CANCELLED" : "APPROVED")
            : "REJECTED";
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << std::setw(10) << status << "  " << std::setw(6)
           << rec.resourceId << " x" << std::setw(3) << rec.quantity << " "
           << "pre-tax: " << std::setw(10) << std::right << rec.preTaxCost.toString()
           << " post-tax: " << std::setw(10) << rec.cost.toString() << "  "
           << "vendor: " << std::setw(10) << std::left << rec.vendor
           << (rec.department.empty() ? "" : " dept: " + rec.department)
           << "  " << rec.title;
        if (!rec.approved || rec.isCancellation) os << "\n        info: " << rec.reason;
        os << "\n";
    }
    os << "Total spent (pre-tax):  " << totalPreTaxSpent() << "\n";
    os << "Total spent (post-tax): " << totalSpent() << "\n";
}

}  // namespace bookmgmt
