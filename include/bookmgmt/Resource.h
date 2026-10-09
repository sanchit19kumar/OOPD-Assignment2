#pragma once
// Resource: abstract base class for every item the library can hold or buy.
//
//   Resource (abstract)
//   ├── Book
//   ├── Journal
//   ├── Thesis
//   └── ElectronicResource
//       ├── EBook
//       └── AudioBook

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "bookmgmt/Money.h"

namespace bookmgmt {

enum class ResourceCategory {
    Book,
    ElectronicResource,
    Journal,
    EBook,
    AudioBook,
    Thesis
};

const char* categoryName(ResourceCategory c);

struct VendorOffer {
    std::string vendor;
    Money price;
};

class Resource {
public:
    Resource(std::string id, std::string title, std::string publisher,
             int year, Money unitPrice);
    virtual ~Resource() = default;

    // Resources are owned polymorphically (std::unique_ptr); no slicing copies.
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& publisher() const { return publisher_; }
    int year() const { return year_; }
    Money unitPrice() const { return unitPrice_; }
    void setUnitPrice(Money price);

    // Vendors (Q12)
    void addVendor(std::string vendor, Money price);
    const std::vector<VendorOffer>& vendors() const { return vendors_; }
    std::optional<VendorOffer> cheapestVendor() const;
    Money effectiveUnitPrice() const;

    virtual ResourceCategory category() const = 0;
    virtual bool isDigital() const { return false; }

    // Total cost of acquiring `quantity` units of this resource.
    // Default handles print bulk discounts (Q5).
    virtual Money costFor(int quantity) const;

    // Writes a multi-line human-readable description.
    void print(std::ostream& os) const;
    // Single-line summary, e.g. "[Book] B001  Clean Code (2008)  @ 450.00".
    std::string summary() const;

    // Search helpers (Q13)
    virtual std::vector<std::string> searchAuthors() const { return {}; }
    virtual std::string searchIdentifier() const { return ""; }

    // Serialization (Q13)
    virtual std::string serialize() const = 0;

protected:
    // Subclasses append their own fields (one "  key: value" line each).
    virtual void printDetails(std::ostream& os) const;
    static void requirePositive(int quantity);

private:
    std::string id_;
    std::string title_;
    std::string publisher_;
    int year_;
    Money unitPrice_;
    std::vector<VendorOffer> vendors_;
};

std::ostream& operator<<(std::ostream& os, const Resource& r);

}  // namespace bookmgmt
