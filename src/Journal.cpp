#include "bookmgmt/Journal.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string publisher,
                 int year, Money annualUnitPrice, std::string issn,
                 int issuesPerYear, int subscriptionYears)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, annualUnitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (subscriptionYears_ < 1) {
        throw std::invalid_argument("subscription length must be at least 1 year");
    }
    if (issuesPerYear_ <= 0) {
        throw std::invalid_argument("issues per year must be positive");
    }
    if (issn_.empty()) {
        throw std::invalid_argument("issn must not be empty");
    }
}

Money Journal::costFor(int copies) const {
    requirePositive(copies);
    Money unit = effectiveUnitPrice();
    std::int64_t total = unit.minorUnits() * copies * subscriptionYears_;
    // Q5: Bulk discount: Print items get 10% off when 10 or more copies are bought at once
    if (copies >= 10) {
        total = total - (total * 10) / 100;
    }
    return Money::fromMinor(total, unit.currency());
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

std::string Journal::serialize() const {
    std::ostringstream os;
    os << "Journal|" << id() << "|" << title() << "|" << publisher() << "|"
       << year() << "|" << unitPrice().minorUnits() << "|" << unitPrice().currency() << "|"
       << issn_ << "|" << issuesPerYear_ << "|" << subscriptionYears_;
    return os.str();
}

}  // namespace bookmgmt
