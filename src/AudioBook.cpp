#include "bookmgmt/AudioBook.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

AudioBook::AudioBook(std::string id, std::string title, std::string publisher,
                     int year, Money pricePerSeat, std::string accessUrl,
                     std::string narrator, int durationMinutes,
                     LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title), std::move(publisher), year,
                         pricePerSeat, std::move(accessUrl), license, platformFee),
      narrator_(std::move(narrator)),
      durationMinutes_(durationMinutes) {
    if (narrator_.empty()) {
        throw std::invalid_argument("narrator must not be empty");
    }
    if (durationMinutes_ <= 0) {
        throw std::invalid_argument("duration must be positive");
    }
}

void AudioBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);
    os << "  narrator: " << narrator_ << "\n"
       << "  duration: " << durationMinutes_ << " mins\n";
}

std::string AudioBook::serialize() const {
    std::ostringstream os;
    os << "AudioBook|" << id() << "|" << title() << "|" << publisher() << "|"
       << year() << "|" << unitPrice().minorUnits() << "|" << unitPrice().currency() << "|"
       << accessUrl() << "|" << (license() == LicenseModel::Perpetual ? "Perpetual" : "AnnualSubscription")
       << "|" << platformFee().minorUnits() << "|" << narrator_ << "|" << durationMinutes_;
    return os.str();
}

}  // namespace bookmgmt
