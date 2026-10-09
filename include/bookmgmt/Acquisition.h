#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing
// Budget quotas, updating Catalog holdings, handling taxes, departments,
// vendors, cancellations, and order history.

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;               // copies for print, seats for electronic
    std::string department = ""; // Q9: Department name (optional)
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money cost;                 // Post-tax total charged
    bool approved;
    std::string reason;         // Rejection or cancellation explanation
    Money preTaxCost;           // Q6: Pre-tax cost
    std::string vendor = "Direct"; // Q12: Vendor used
    std::string department = "";   // Q9: Department charged
    bool isCancellation = false;   // Q8: Whether this is a cancellation record
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget);

    // Q6: Taxes configuration
    void setTaxRates(double printRate, double electronicRate);
    double printTaxRate() const { return printTaxRate_; }
    double electronicTaxRate() const { return electronicTaxRate_; }
    Money calculatePostTax(const Resource& r, Money preTax) const;

    // Q9: Department Budgets
    void setDepartmentBudget(const std::string& dept, Budget budget);
    Budget* departmentBudget(const std::string& dept);
    const Budget* departmentBudget(const std::string& dept) const;

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;
    Money quotePreTax(const std::string& id, int quantity) const;

    // True if purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr,
                     const std::string& department = "") const;

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument.
    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                   const std::string& department = "");

    // Q8: Cancellation - refund budget and quota usage, reduce holdings
    const PurchaseRecord& cancelOrder(int orderNo);

    // Q11: processBatch with allOrNothing option
    std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest>& reqs,
                                             bool allOrNothing = false);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;
    Money totalPreTaxSpent() const;

    void printReport(std::ostream& os) const;

private:
    Budget& getBudgetFor(const std::string& department);
    const Budget& getBudgetFor(const std::string& department) const;

    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                           Money postTaxCost, Money preTaxCost, bool approved,
                           std::string reason, std::string vendor = "Direct",
                           std::string dept = "", bool isCancel = false);

    Catalog& catalog_;
    Budget& budget_;
    std::map<std::string, Budget> departmentBudgets_;
    double printTaxRate_ = 0.0;
    double electronicTaxRate_ = 0.0;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
