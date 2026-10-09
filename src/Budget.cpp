#include "bookmgmt/Budget.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::ElectronicResource,
    ResourceCategory::Journal,
    ResourceCategory::EBook,
    ResourceCategory::AudioBook,
    ResourceCategory::Thesis
};
}  // namespace

Budget::Budget(Money total) : total_(total) {
    if (total_.isNegative()) throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative())
        throw std::invalid_argument("quota limits must not be negative");
    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);
    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxSpend - usageFor(c).spent;
}

std::optional<int> Budget::titlesRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q || q->maxTitles < 0) return std::nullopt;
    int used = 0;
    auto it = titlesBought_.find(c);
    if (it != titlesBought_.end()) used = static_cast<int>(it->second.size());
    return q->maxTitles - used;
}

Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                 const std::string& titleId, std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }
    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }
    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) + " unit quota exceeded: requested " +
              std::to_string(units) + ", " + std::to_string(*left) + " remaining";
        return Failure::Quota;
    }
    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) + " spend quota exceeded: cost " +
              cost.toString() + ", " + left->toString() + " remaining";
        return Failure::Quota;
    }
    // Q7: Title quota check
    if (!titleId.empty()) {
        auto q = quotaFor(c);
        if (q && q->maxTitles >= 0) {
            bool alreadyBought = false;
            auto it = titlesBought_.find(c);
            if (it != titlesBought_.end() && it->second.count(titleId) > 0) {
                alreadyBought = true;
            }
            if (!alreadyBought) {
                int currentTitles = (it != titlesBought_.end()) ? static_cast<int>(it->second.size()) : 0;
                if (currentTitles + 1 > q->maxTitles) {
                    why = std::string(categoryName(c)) + " title quota exceeded: max " +
                          std::to_string(q->maxTitles) + " different titles allowed";
                    return Failure::Quota;
                }
            }
        }
    }
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " + cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }
    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory c, int units, Money cost,
                          const std::string& titleId) const {
    std::string why;
    evaluate(c, units, cost, titleId, why);
    return why;
}

void Budget::commit(ResourceCategory c, int units, Money cost,
                    const std::string& titleId) {
    std::string why;
    switch (evaluate(c, units, cost, titleId, why)) {
        case Failure::None: break;
        case Failure::BadInput: throw std::invalid_argument(why);
        case Failure::Quota: throw QuotaExceededError(why);
        case Failure::Overall: throw BudgetExceededError(why);
    }
    Usage& u = usage_[c];
    u.units += units;
    u.spent += cost;
    if (!titleId.empty()) {
        titlesBought_[c].insert(titleId);
        u.titles = static_cast<int>(titlesBought_[c].size());
    }
    spent_ += cost;

    // Q10: 80% quota warning
    std::string warning = quotaWarning(c, 0.80);
    if (!warning.empty()) {
        std::clog << "[WARNING] " << warning << "\n";
    }
}

void Budget::refund(ResourceCategory c, int units, Money cost,
                    const std::string& /*titleId*/) {
    if (units < 0 || cost.isNegative()) {
        throw std::invalid_argument("refund units and cost must not be negative");
    }
    spent_ -= cost;
    Usage& u = usage_[c];
    u.units = std::max(0, u.units - units);
    u.spent -= cost;
}

Budget Budget::createNextYearBudget(double carryForwardPercent,
                                    Money newBaseAllocation) const {
    if (carryForwardPercent < 0.0 || carryForwardPercent > 100.0) {
        throw std::invalid_argument("carry-forward percentage must be between 0 and 100");
    }
    std::int64_t unspentMinor = remaining().minorUnits();
    std::int64_t carryMinor = static_cast<std::int64_t>(std::llround(unspentMinor * (carryForwardPercent / 100.0)));
    Money carryMoney = Money::fromMinor(carryMinor, remaining().currency());
    Money nextTotal = newBaseAllocation + carryMoney;

    Budget nextYear(nextTotal);
    for (const auto& [cat, q] : quotas_) {
        nextYear.setQuota(cat, q);
    }
    return nextYear;
}

bool Budget::isPastQuotaThreshold(ResourceCategory c, double threshold) const {
    auto q = quotaFor(c);
    if (!q) return false;
    const Usage u = usageFor(c);
    if (q->maxUnits > 0 && static_cast<double>(u.units) / q->maxUnits >= threshold) return true;
    if (!q->maxSpend.isZero() &&
        static_cast<double>(u.spent.minorUnits()) / q->maxSpend.minorUnits() >= threshold) return true;
    return false;
}

std::string Budget::quotaWarning(ResourceCategory c, double threshold) const {
    auto q = quotaFor(c);
    if (!q) return "";
    const Usage u = usageFor(c);
    std::string w;
    if (q->maxUnits > 0 && static_cast<double>(u.units) / q->maxUnits >= threshold) {
        w += std::string(categoryName(c)) + " has reached " +
             std::to_string(static_cast<int>(std::round(100.0 * u.units / q->maxUnits))) +
             "% of its unit quota (" + std::to_string(u.units) + "/" +
             std::to_string(q->maxUnits) + "). ";
    }
    if (!q->maxSpend.isZero() &&
        static_cast<double>(u.spent.minorUnits()) / q->maxSpend.minorUnits() >= threshold) {
        w += std::string(categoryName(c)) + " has reached " +
             std::to_string(static_cast<int>(std::round(100.0 * u.spent.minorUnits() / q->maxSpend.minorUnits()))) +
             "% of its spend quota (" + u.spent.toString() + "/" +
             q->maxSpend.toString() + ").";
    }
    return w;
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
       << std::setw(20) << "Spend used/max" << "Titles used/max\n";
    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);
        const std::string units =
            std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
        const std::string spend =
            u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
        const std::string titles =
            std::to_string(u.titles) + "/" +
            (q && q->maxTitles >= 0 ? std::to_string(q->maxTitles) : "-");
        os << "  " << std::setw(20) << categoryName(c) << std::setw(18) << units
           << std::setw(20) << spend << titles << "\n";
    }
}

}  // namespace bookmgmt
