#pragma once
// Catalog: owns every Resource, keyed by its id, and tracks copies/seats held
// and active loans/sessions.

#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class Catalog {
public:
    // Takes ownership. Throws DuplicateIdError if the id is already present.
    Resource& add(std::unique_ptr<Resource> r);

    // Convenience: catalog.emplace<Book>(ctor args...)
    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *p;
        add(std::move(p));
        return ref;
    }

    bool contains(const std::string& id) const;
    Resource* find(const std::string& id);              // nullptr if absent
    const Resource* find(const std::string& id) const;  // nullptr if absent
    Resource& get(const std::string& id);               // throws NotFoundError
    const Resource& get(const std::string& id) const;   // throws NotFoundError
    void remove(const std::string& id);                 // throws NotFoundError
    void clear();

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    // Holdings: number of copies (print) or seats (electronic) owned.
    int holdings(const std::string& id) const;
    void addHoldings(const std::string& id, int units);

    // Q14: Lending (print copies) and Sessions (electronic seats)
    int activeLoans(const std::string& id) const;
    int availableUnits(const std::string& id) const;
    void borrowItem(const std::string& id);
    void returnItem(const std::string& id);
    void openSession(const std::string& id);
    void closeSession(const std::string& id);

    // Queries. Results are in id order and point into the catalog.
    std::vector<const Resource*> all() const;
    std::vector<const Resource*> byCategory(ResourceCategory c) const;
    std::vector<const Resource*> searchTitle(const std::string& text) const;  // case-insensitive
    std::vector<const Resource*> where(
        const std::function<bool(const Resource&)>& pred) const;

    // Q13: Extra searches
    std::vector<const Resource*> searchAuthor(const std::string& author) const;
    std::vector<const Resource*> searchIsbnOrIssn(const std::string& code) const;
    std::vector<const Resource*> searchYearRange(int startYear, int endYear) const;

    // Q13: Save / Load catalog to / from stream and file
    void save(std::ostream& os) const;
    void load(std::istream& is);
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);

private:
    struct Entry {
        std::unique_ptr<Resource> resource;
        int holdings = 0;
        int activeLoans = 0;  // borrowed copies (print) or open sessions (digital)
    };
    std::map<std::string, Entry> items_;
};

}  // namespace bookmgmt
