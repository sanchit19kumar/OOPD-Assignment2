#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats), how much money may be spent,
// and optionally how many distinct titles may be bought in one category.

#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct Quota {
    int maxUnits = 0;    // maximum copies/seats that may be bought
    Money maxSpend;      // maximum money that may be spent
    int maxTitles = -1;  // Q7: maximum distinct titles (-1 means unlimited)

    Quota() = default;
    Quota(int units, Money spend, int titles = -1)
        : maxUnits(units), maxSpend(spend), maxTitles(titles) {}
};

struct Usage {
    int units = 0;
    Money spent;
    int titles = 0;      // distinct titles purchased
};

class Budget {
public:
    explicit Budget(Money total);

    Money total() const { return total_; }
    Money spent() const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory c, Quota q);
    void removeQuota(ResourceCategory c);
    std::optional<Quota> quotaFor(ResourceCategory c) const;
    Usage usageFor(ResourceCategory c) const;

    // Remaining allowance in a category; nullopt means "no quota set".
    std::optional<int> unitsRemaining(ResourceCategory c) const;
    std::optional<Money> spendRemaining(ResourceCategory c) const;
    std::optional<int> titlesRemaining(ResourceCategory c) const;

    // Returns an empty string if the purchase fits, otherwise the reason it does not.
    // Supports title limit checking (Q7).
    std::string check(ResourceCategory c, int units, Money cost,
                      const std::string& titleId = "") const;

    // Records a purchase. Throws QuotaExceededError / BudgetExceededError if it does not fit.
    void commit(ResourceCategory c, int units, Money cost,
                const std::string& titleId = "");

    // Q8: Cancellation - refund budget and quota usage
    void refund(ResourceCategory c, int units, Money cost,
                const std::string& titleId = "");

    // Q10: Year-end rollover: carries forward a percentage of unspent money
    Budget createNextYearBudget(double carryForwardPercent,
                               Money newBaseAllocation = Money{}) const;

    // Q10: Warnings when a category passes threshold (e.g. 80%) of unit or spend quota
    bool isPastQuotaThreshold(ResourceCategory c, double threshold = 0.80) const;
    std::string quotaWarning(ResourceCategory c, double threshold = 0.80) const;

    void print(std::ostream& os) const;

private:
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory c, int units, Money cost,
                     const std::string& titleId, std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota> quotas_;
    std::map<ResourceCategory, Usage> usage_;
    std::map<ResourceCategory, std::set<std::string>> titlesBought_;
};

}  // namespace bookmgmt
