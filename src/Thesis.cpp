#include "bookmgmt/Thesis.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

Thesis::Thesis(std::string id, std::string title, std::string university,
               std::string degree, std::string supervisor, int year,
               Money unitPrice, std::string publisher)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      university_(std::move(university)),
      degree_(std::move(degree)),
      supervisor_(std::move(supervisor)) {
    if (university_.empty()) throw std::invalid_argument("university must not be empty");
    if (degree_.empty()) throw std::invalid_argument("degree must not be empty");
    if (supervisor_.empty()) throw std::invalid_argument("supervisor must not be empty");
}

void Thesis::printDetails(std::ostream& os) const {
    os << "  university: " << university_ << "\n"
       << "  degree: " << degree_ << "\n"
       << "  supervisor: " << supervisor_ << "\n";
}

std::string Thesis::serialize() const {
    std::ostringstream os;
    os << "Thesis|" << id() << "|" << title() << "|" << publisher() << "|"
       << year() << "|" << unitPrice().minorUnits() << "|" << unitPrice().currency() << "|"
       << university_ << "|" << degree_ << "|" << supervisor_;
    return os.str();
}

}  // namespace bookmgmt
