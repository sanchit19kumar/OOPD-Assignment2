#include "bookmgmt/ElectronicResource.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

const char* licenseName(LicenseModel m) {
    switch (m) {
        case LicenseModel::Perpetual: return "Perpetual";
        case LicenseModel::AnnualSubscription: return "Annual subscription";
    }
    return "Unknown";
}

ElectronicResource::ElectronicResource(std::string id, std::string title,
                                       std::string publisher, int year,
                                       Money pricePerSeat, std::string accessUrl,
                                       LicenseModel license, Money platformFee)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, pricePerSeat),
      accessUrl_(std::move(accessUrl)),
      license_(license),
      platformFee_(platformFee) {
    if (platformFee_.isNegative())
        throw std::invalid_argument("platform fee must not be negative");
}

Money ElectronicResource::costFor(int seats) const {
    requirePositive(seats);
    Money unit = effectiveUnitPrice();
    std::int64_t unitMinor = unit.minorUnits();
    std::int64_t seatCost = 0;
    // Q5: For electronic resources, every seat beyond the 50th costs half price
    if (seats <= 50) {
        seatCost = unitMinor * seats;
    } else {
        std::int64_t beyond = seats - 50;
        seatCost = unitMinor * 50 + (unitMinor * beyond) / 2;
    }
    return platformFee_ + Money::fromMinor(seatCost, unit.currency());
}

void ElectronicResource::printDetails(std::ostream& os) const {
    os << "  access url: " << accessUrl_ << "\n"
       << "  license: " << licenseName(license_) << "\n"
       << "  platform fee: " << platformFee_ << "\n";
}

std::string ElectronicResource::serialize() const {
    std::ostringstream os;
    os << "ElectronicResource|" << id() << "|" << title() << "|" << publisher() << "|"
       << year() << "|" << unitPrice().minorUnits() << "|" << unitPrice().currency() << "|"
       << accessUrl_ << "|" << (license_ == LicenseModel::Perpetual ? "Perpetual" : "AnnualSubscription")
       << "|" << platformFee_.minorUnits();
    return os.str();
}

}  // namespace bookmgmt
