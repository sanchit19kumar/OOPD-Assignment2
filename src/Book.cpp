#include "bookmgmt/Book.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

std::string joinAuthors(const std::vector<std::string>& authors) {
    std::string out;
    for (std::size_t i = 0; i < authors.size(); ++i) {
        if (i > 0) out += (i + 1 == authors.size()) ? " and " : ", ";
        out += authors[i];
    }
    return out;
}

Book::Book(std::string id, std::string title, std::vector<std::string> authors,
           std::string isbn, std::string publisher, int year, Money unitPrice,
           int edition, Binding binding)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      edition_(edition),
      binding_(binding) {
    if (edition_ < 1) throw std::invalid_argument("edition must be >= 1");
}

Money Book::costFor(int quantity) const {
    requirePositive(quantity);
    Money unit = effectiveUnitPrice();
    std::int64_t baseMinor = unit.minorUnits();
    // Q4: Hardcover books cost 20% more than their listed unit price
    if (binding_ == Binding::Hardcover) {
        baseMinor = (baseMinor * 120) / 100;
    }
    std::int64_t total = baseMinor * quantity;
    // Q5: Print items get 10% off when 10 or more copies are bought at once
    if (quantity >= 10) {
        total = total - (total * 10) / 100;
    }
    return Money::fromMinor(total, unit.currency());
}

void Book::printDetails(std::ostream& os) const {
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  edition: " << edition_ << "\n"
       << "  binding: " << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback")
       << "\n";
}

std::string Book::serialize() const {
    std::ostringstream os;
    os << "Book|" << id() << "|" << title() << "|" << publisher() << "|"
       << year() << "|" << unitPrice().minorUnits() << "|" << unitPrice().currency() << "|";
    for (std::size_t i = 0; i < authors_.size(); ++i) {
        if (i > 0) os << ";";
        os << authors_[i];
    }
    os << "|" << isbn_ << "|" << edition_ << "|"
       << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback");
    return os.str();
}

}  // namespace bookmgmt
