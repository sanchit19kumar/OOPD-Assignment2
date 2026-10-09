#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "bookmgmt/AudioBook.h"
#include "bookmgmt/Book.h"
#include "bookmgmt/EBook.h"
#include "bookmgmt/ElectronicResource.h"
#include "bookmgmt/Exceptions.h"
#include "bookmgmt/Journal.h"
#include "bookmgmt/Thesis.h"

namespace bookmgmt {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}
}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> r) {
    if (!r) throw std::invalid_argument("cannot add a null resource");
    const std::string id = r->id();
    if (items_.count(id)) throw DuplicateIdError(id);
    Entry& e = items_[id];
    e.resource = std::move(r);
    return *e.resource;
}

bool Catalog::contains(const std::string& id) const { return items_.count(id) > 0; }

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    if (Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    if (const Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    if (items_.erase(id) == 0) throw NotFoundError(id);
}

void Catalog::clear() {
    items_.clear();
}

int Catalog::holdings(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.holdings + units < 0)
        throw std::invalid_argument("holdings cannot become negative");
    it->second.holdings += units;
}

int Catalog::activeLoans(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.activeLoans;
}

int Catalog::availableUnits(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings - it->second.activeLoans;
}

void Catalog::borrowItem(const std::string& id) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.resource->isDigital())
        throw std::invalid_argument("cannot borrow electronic resource as print copy; open a session instead");
    if (it->second.activeLoans >= it->second.holdings)
        throw LibraryError("no print copies available for loan: " + id);
    ++it->second.activeLoans;
}

void Catalog::returnItem(const std::string& id) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.activeLoans <= 0)
        throw std::invalid_argument("no active loans to return: " + id);
    --it->second.activeLoans;
}

void Catalog::openSession(const std::string& id) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (!it->second.resource->isDigital())
        throw std::invalid_argument("cannot open electronic session on print resource; borrow print copy instead");
    if (it->second.activeLoans >= it->second.holdings)
        throw LibraryError("all licensed concurrent seats are in use: " + id);
    ++it->second.activeLoans;
}

void Catalog::closeSession(const std::string& id) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.activeLoans <= 0)
        throw std::invalid_argument("no active sessions to close: " + id);
    --it->second.activeLoans;
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& pred) const {
    std::vector<const Resource*> out;
    for (const auto& [id, entry] : items_)
        if (pred(*entry.resource)) out.push_back(entry.resource.get());
    return out;
}

std::vector<const Resource*> Catalog::all() const {
    return where([](const Resource&) { return true; });
}

std::vector<const Resource*> Catalog::byCategory(ResourceCategory c) const {
    return where([c](const Resource& r) { return r.category() == c; });
}

std::vector<const Resource*> Catalog::searchTitle(const std::string& text) const {
    const std::string needle = lower(text);
    return where([&needle](const Resource& r) {
        return lower(r.title()).find(needle) != std::string::npos;
    });
}

std::vector<const Resource*> Catalog::searchAuthor(const std::string& author) const {
    const std::string needle = lower(author);
    return where([&needle](const Resource& r) {
        for (const auto& a : r.searchAuthors()) {
            if (lower(a).find(needle) != std::string::npos) return true;
        }
        return false;
    });
}

std::vector<const Resource*> Catalog::searchIsbnOrIssn(const std::string& code) const {
    return where([&code](const Resource& r) {
        return r.searchIdentifier() == code;
    });
}

std::vector<const Resource*> Catalog::searchYearRange(int startYear, int endYear) const {
    return where([startYear, endYear](const Resource& r) {
        return r.year() >= startYear && r.year() <= endYear;
    });
}

void Catalog::save(std::ostream& os) const {
    for (const auto& [id, entry] : items_) {
        os << entry.holdings << "|" << entry.resource->serialize() << "\n";
    }
}

void Catalog::load(std::istream& is) {
    std::string line;
    while (std::getline(is, line)) {
        if (line.empty()) continue;
        auto tokens = split(line, '|');
        if (tokens.size() < 8) continue;
        int h = std::stoi(tokens[0]);
        std::string type = tokens[1];
        std::string id = tokens[2];
        std::string title = tokens[3];
        std::string publisher = tokens[4];
        int year = std::stoi(tokens[5]);
        std::int64_t minorPrice = std::stoll(tokens[6]);
        std::string currency = tokens[7];
        Money price = Money::fromMinor(minorPrice, currency);

        if (type == "Book" && tokens.size() >= 12) {
            std::vector<std::string> authors = split(tokens[8], ';');
            std::string isbn = tokens[9];
            int edition = std::stoi(tokens[10]);
            Binding binding = (tokens[11] == "Hardcover" ? Binding::Hardcover : Binding::Paperback);
            emplace<Book>(id, title, authors, isbn, publisher, year, price, edition, binding);
            if (h > 0) addHoldings(id, h);
        } else if (type == "Journal" && tokens.size() >= 11) {
            std::string issn = tokens[8];
            int issues = std::stoi(tokens[9]);
            int subYears = std::stoi(tokens[10]);
            emplace<Journal>(id, title, publisher, year, price, issn, issues, subYears);
            if (h > 0) addHoldings(id, h);
        } else if (type == "ElectronicResource" && tokens.size() >= 11) {
            std::string accessUrl = tokens[8];
            LicenseModel license = (tokens[9] == "Perpetual" ? LicenseModel::Perpetual : LicenseModel::AnnualSubscription);
            Money platformFee = Money::fromMinor(std::stoll(tokens[10]), currency);
            emplace<ElectronicResource>(id, title, publisher, year, price, accessUrl, license, platformFee);
            if (h > 0) addHoldings(id, h);
        } else if (type == "EBook" && tokens.size() >= 15) {
            std::string accessUrl = tokens[8];
            LicenseModel license = (tokens[9] == "Perpetual" ? LicenseModel::Perpetual : LicenseModel::AnnualSubscription);
            Money platformFee = Money::fromMinor(std::stoll(tokens[10]), currency);
            std::vector<std::string> authors = split(tokens[11], ';');
            std::string isbn = tokens[12];
            std::string format = tokens[13];
            bool drm = (tokens[14] == "1");
            emplace<EBook>(id, title, authors, isbn, publisher, year, price, accessUrl,
                           format, drm, license, platformFee);
            if (h > 0) addHoldings(id, h);
        } else if (type == "AudioBook" && tokens.size() >= 13) {
            std::string accessUrl = tokens[8];
            LicenseModel license = (tokens[9] == "Perpetual" ? LicenseModel::Perpetual : LicenseModel::AnnualSubscription);
            Money platformFee = Money::fromMinor(std::stoll(tokens[10]), currency);
            std::string narrator = tokens[11];
            int duration = std::stoi(tokens[12]);
            emplace<AudioBook>(id, title, publisher, year, price, accessUrl,
                               narrator, duration, license, platformFee);
            if (h > 0) addHoldings(id, h);
        } else if (type == "Thesis" && tokens.size() >= 11) {
            std::string university = tokens[8];
            std::string degree = tokens[9];
            std::string supervisor = tokens[10];
            emplace<Thesis>(id, title, university, degree, supervisor, year, price, publisher);
            if (h > 0) addHoldings(id, h);
        }
    }
}

void Catalog::saveToFile(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (!ofs) throw std::runtime_error("failed to open file for writing: " + filename);
    save(ofs);
}

void Catalog::loadFromFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) throw std::runtime_error("failed to open file for reading: " + filename);
    load(ifs);
}

}  // namespace bookmgmt
