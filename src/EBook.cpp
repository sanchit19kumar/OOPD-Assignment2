#include "bookmgmt/EBook.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

EBook::EBook(std::string id, std::string title, std::vector<std::string> authors,
             std::string isbn, std::string publisher, int year, Money pricePerSeat,
             std::string accessUrl, std::string format, bool isDrmProtected,
             LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title), std::move(publisher), year,
                         pricePerSeat, std::move(accessUrl), license, platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(std::move(format)),
      isDrmProtected_(isDrmProtected) {
    if (format_ != "PDF" && format_ != "EPUB" && format_ != "HTML") {
        throw std::invalid_argument("file format must be PDF, EPUB or HTML");
    }
}

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  format: " << format_ << "\n"
       << "  drm protected: " << (isDrmProtected_ ? "yes" : "no") << "\n";
}

std::string EBook::serialize() const {
    std::ostringstream os;
    os << "EBook|" << id() << "|" << title() << "|" << publisher() << "|"
       << year() << "|" << unitPrice().minorUnits() << "|" << unitPrice().currency() << "|"
       << accessUrl() << "|" << (license() == LicenseModel::Perpetual ? "Perpetual" : "AnnualSubscription")
       << "|" << platformFee().minorUnits() << "|";
    for (std::size_t i = 0; i < authors_.size(); ++i) {
        if (i > 0) os << ";";
        os << authors_[i];
    }
    os << "|" << isbn_ << "|" << format_ << "|" << (isDrmProtected_ ? "1" : "0");
    return os.str();
}

}  // namespace bookmgmt
