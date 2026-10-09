#pragma once

#include <string>
#include <vector>

#include "bookmgmt/Book.h"
#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

/*
 * DESIGN QUESTION (Q2):
 * "An e-book is also a kind of book. Which code is now duplicated between
 *  Book and EBook, and how could you avoid the duplication?"
 *
 * ANSWER:
 * 1. Duplicated Code:
 *    - Data members: std::vector<std::string> authors_ and std::string isbn_.
 *    - Member functions: authors() accessor, isbn() accessor, searchAuthors(),
 *      and searchIdentifier().
 *    - Formatting: joinAuthors() call and printing of authors and ISBN in printDetails().
 *    - Constructor validation logic for authors/ISBN.
 *
 * 2. How to avoid the duplication:
 *    - Composition (Preferred in modern C++):
 *      Extract a `BookMetadata` or `BibliographicInfo` class/struct holding
 *      authors, ISBN, edition, etc. Both `Book` and `EBook` would compose an instance
 *      of `BookMetadata`, delegating author/ISBN queries and printing to it.
 *    - Multiple Inheritance / Mixins:
 *      Create an abstract `PrintableBook` or `BookIdentity` interface/mixin that defines
 *      authors and ISBN handling. `Book` inherits from `Resource` and `BookIdentity`,
 *      while `EBook` inherits from `ElectronicResource` and `BookIdentity`.
 *    - Hierarchy Restructuring (Bridge or Decoupled Dimensions):
 *      Separate the "Content" hierarchy (Book, Journal, Thesis, Music) from the
 *      "Delivery/Licensing Medium" hierarchy (Physical Copy vs Electronic Concurrent Seat),
 *      using the Bridge Design Pattern or strategy pattern for licensing.
 */

class EBook : public ElectronicResource {
public:
    EBook(std::string id, std::string title, std::vector<std::string> authors,
          std::string isbn, std::string publisher, int year, Money pricePerSeat,
          std::string accessUrl, std::string format = "PDF", bool isDrmProtected = false,
          LicenseModel license = LicenseModel::AnnualSubscription,
          Money platformFee = Money{});

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    const std::string& format() const { return format_; }
    bool isDrmProtected() const { return isDrmProtected_; }

    ResourceCategory category() const override { return ResourceCategory::EBook; }

    std::vector<std::string> searchAuthors() const override { return authors_; }
    std::string searchIdentifier() const override { return isbn_; }
    std::string serialize() const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    std::string format_;
    bool isDrmProtected_;
};

}  // namespace bookmgmt
