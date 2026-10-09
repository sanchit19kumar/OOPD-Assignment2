#include "bookmgmt/Resource.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

const char* categoryName(ResourceCategory c) {
    switch (c) {
        case ResourceCategory::Book: return "Book";
        case ResourceCategory::ElectronicResource: return "ElectronicResource";
        case ResourceCategory::Journal: return "Journal";
        case ResourceCategory::EBook: return "EBook";
        case ResourceCategory::AudioBook: return "AudioBook";
        case ResourceCategory::Thesis: return "Thesis";
    }
    return "Unknown";
}

Resource::Resource(std::string id, std::string title, std::string publisher,
                   int year, Money unitPrice)
    : id_(std::move(id)),
      title_(std::move(title)),
      publisher_(std::move(publisher)),
      year_(year),
      unitPrice_(unitPrice) {
    if (id_.empty()) throw std::invalid_argument("resource id must not be empty");
    if (title_.empty()) throw std::invalid_argument("resource title must not be empty");
    if (unitPrice_.isNegative()) throw std::invalid_argument("price must not be negative");
}

void Resource::setUnitPrice(Money price) {
    if (price.isNegative()) throw std::invalid_argument("price must not be negative");
    unitPrice_ = price;
}

void Resource::addVendor(std::string vendor, Money price) {
    if (vendor.empty()) throw std::invalid_argument("vendor name must not be empty");
    if (price.isNegative()) throw std::invalid_argument("vendor price must not be negative");
    vendors_.push_back(VendorOffer{std::move(vendor), price});
}

std::optional<VendorOffer> Resource::cheapestVendor() const {
    if (vendors_.empty()) return std::nullopt;
    auto it = std::min_element(vendors_.begin(), vendors_.end(),
                               [](const VendorOffer& a, const VendorOffer& b) {
                                   return a.price < b.price;
                               });
    return *it;
}

Money Resource::effectiveUnitPrice() const {
    auto best = cheapestVendor();
    if (best) return best->price;
    return unitPrice_;
}

void Resource::requirePositive(int quantity) {
    if (quantity <= 0) throw std::invalid_argument("quantity must be positive");
}

Money Resource::costFor(int quantity) const {
    requirePositive(quantity);
    Money unit = effectiveUnitPrice();
    std::int64_t total = unit.minorUnits() * quantity;
    // Q5: Print items get 10% off when 10 or more copies are bought at once
    if (!isDigital() && quantity >= 10) {
        total = total - (total * 10) / 100;
    }
    return Money::fromMinor(total, unit.currency());
}

void Resource::print(std::ostream& os) const {
    os << categoryName(category()) << " " << id_ << "\n"
       << "  title: " << title_ << "\n"
       << "  publisher: " << publisher_ << "\n"
       << "  year: " << year_ << "\n"
       << "  unit price: " << unitPrice_ << "\n";
    if (!vendors_.empty()) {
        os << "  vendors:\n";
        for (const auto& v : vendors_) {
            os << "    " << v.vendor << " @ " << v.price << "\n";
        }
    }
    printDetails(os);
}

void Resource::printDetails(std::ostream&) const {}

std::string Resource::summary() const {
    std::ostringstream os;
    os << "[" << categoryName(category()) << "] " << id_ << "  " << title_
       << " (" << year_ << ")  @ " << effectiveUnitPrice();
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Resource& r) {
    r.print(os);
    return os;
}

}  // namespace bookmgmt
