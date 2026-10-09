// Test suite using standard C++ assert (Q16) - no macro-based test framework.

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

// Q16: Helper template function using assert to test for expected exceptions without macros.
template <typename ExceptionType, typename Func>
static void assertThrows(Func&& fn) {
    bool caught = false;
    try {
        fn();
    } catch (const ExceptionType&) {
        caught = true;
    } catch (...) {
        caught = false;
    }
    assert(caught && "Expected exception of specified type was not thrown");
}

static void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assertThrows<std::invalid_argument>([]() { Money::of(1, 100); });

    // Q15: Currency code and currency mismatch tests
    Money inr1 = Money::of(100, 0, "INR");
    Money inr2 = Money::of(50, 0, "INR");
    Money usd1 = Money::of(100, 0, "USD");
    Money usd2 = Money::of(50, 0, "USD");
    Money eur1 = Money::of(100, 0, "EUR");

    assert(inr1.currency() == "INR");
    assert(usd1.currency() == "USD");
    assert(inr1 + inr2 == Money::of(150, 0, "INR"));
    assert(usd1 - usd2 == Money::of(50, 0, "USD"));

    // Operations across different currencies must throw
    assertThrows<CurrencyMismatchError>([&]() { (void)(inr1 + usd1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(inr1 - usd1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(usd1 == eur1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(usd1 != eur1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(usd1 < eur1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(usd1 <= eur1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(usd1 > eur1); });
    assertThrows<CurrencyMismatchError>([&]() { (void)(usd1 >= eur1); });
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert(b.category() == ResourceCategory::Book);
    assert(!b.isDigital());
    assert(b.costFor(3) == Money::of(300));
    assertThrows<std::invalid_argument>([&]() { b.costFor(0); });
    assert(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert(e.isDigital());
    assert(e.costFor(5) == Money::of(150));
    assert(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    assert(os.str().find("platform fee: 100.00") != std::string::npos);

    assertThrows<std::invalid_argument>([]() {
        Book("", "T", {}, "", "", 2000, Money::of(1));
    });
    assertThrows<std::invalid_argument>([]() {
        Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1));
    });
}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    assert(c.size() == 3);
    assert(c.contains("B1"));
    assert(c.find("nope") == nullptr);
    assertThrows<NotFoundError>([&]() { c.get("nope"); });
    assertThrows<DuplicateIdError>([&]() {
        c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1, Money::of(1));
    });

    assert(c.searchTitle("clean").size() == 2);
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    assert(c.where([](const Resource& res) { return res.isDigital(); }).size() == 1);

    assert(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    assert(c.holdings("B1") == 3);
    assertThrows<std::invalid_argument>([&]() { c.addHoldings("B1", -5); });

    c.remove("R1");
    assert(c.size() == 2);
    assertThrows<NotFoundError>([&]() { c.remove("R1"); });
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    assert(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    assert(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    assert(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    assert(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    assert(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert(b.spent() == Money::of(300));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    assert(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    assertThrows<QuotaExceededError>([&]() {
        b.commit(ResourceCategory::Book, 2, Money::of(10));
    });
    assertThrows<BudgetExceededError>([&]() {
        b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800));
    });
    assertThrows<std::invalid_argument>([&]() {
        b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1));
    });
    assert(b.spent() == Money::of(300));  // failed commits changed nothing
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    assert(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    assert(acq.canPurchase("B1", 3, &why) && why.empty());
    assert(!acq.canPurchase("B1", 4, &why) && !why.empty());
    assert(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    assert(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    assert(c.holdings("B1") == 2);

    assertThrows<QuotaExceededError>([&]() { acq.purchase("B1", 2); });
    assertThrows<NotFoundError>([&]() { acq.purchase("nope", 1); });
    assert(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert(res.size() == 5);
    assert(res[0].approved && res[0].cost == Money::of(150));
    assert(!res[1].approved);  // 1050 > remaining 150
    assert(res[2].approved);
    assert(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    assert(!res[4].approved);
    assert(acq.totalSpent() == Money::of(450));
    assert(b.spent() == acq.totalSpent());
    assert(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    assert(acq.history().size() == 6);
}

// ==================== TESTS FOR NEW FEATURES (Q1 to Q15) ====================

static void testQ1Journal() {
    Journal j("J1", "Nature", "Springer", 2026, Money::of(300), "0028-0836", 52, 2);
    assert(j.category() == ResourceCategory::Journal);
    assert(!j.isDigital());
    assert(j.issn() == "0028-0836");
    assert(j.issuesPerYear() == 52);
    assert(j.subscriptionYears() == 2);
    // 300 * 2 copies * 2 years = 1200
    assert(j.costFor(2) == Money::of(1200));

    // Default subscription years = 1
    Journal jDefault("J2", "Science", "AAAS", 2026, Money::of(250), "0036-8075", 50);
    assert(jDefault.subscriptionYears() == 1);
    assert(jDefault.costFor(1) == Money::of(250));

    // Invalid subscription length < 1 throws invalid_argument
    assertThrows<std::invalid_argument>([]() {
        Journal("J3", "Bad", "P", 2026, Money::of(100), "1234-5678", 12, 0);
    });

    std::ostringstream os;
    j.print(os);
    assert(os.str().find("issn: 0028-0836") != std::string::npos);
    assert(os.str().find("issues per year: 52") != std::string::npos);
    assert(os.str().find("subscription years: 2") != std::string::npos);

    // Quota and categoryName
    assert(std::string(categoryName(ResourceCategory::Journal)) == "Journal");
    Budget b(Money::of(2000));
    b.setQuota(ResourceCategory::Journal, {5, Money::of(1500)});
    assert(b.check(ResourceCategory::Journal, 2, Money::of(1200)).empty());
}

static void testQ2EBook() {
    EBook eb("EB1", "Modern C++", {"Scott Meyers"}, "978-1491903995",
             "O'Reilly", 2014, Money::of(20), "https://oreilly.example/cpp",
             "EPUB", true, LicenseModel::Perpetual, Money::of(100));

    assert(eb.category() == ResourceCategory::EBook);
    assert(eb.isDigital());
    assert(eb.format() == "EPUB");
    assert(eb.isDrmProtected());
    assert(eb.authors().size() == 1 && eb.authors()[0] == "Scott Meyers");
    assert(eb.isbn() == "978-1491903995");

    // Pricing inherited from ElectronicResource: platformFee (100) + pricePerSeat (20) * 5 seats = 200
    assert(eb.costFor(5) == Money::of(200));

    // Invalid format throws
    assertThrows<std::invalid_argument>([]() {
        EBook("EB2", "Bad", {"A"}, "isbn", "P", 2026, Money::of(10), "url", "MOBI");
    });

    std::ostringstream os;
    eb.print(os);
    assert(os.str().find("access url: https://oreilly.example/cpp") != std::string::npos);
    assert(os.str().find("authors: Scott Meyers") != std::string::npos);
    assert(os.str().find("format: EPUB") != std::string::npos);
    assert(os.str().find("drm protected: yes") != std::string::npos);
    assert(std::string(categoryName(ResourceCategory::EBook)) == "EBook");
}

static void testQ3AudioBookAndThesis() {
    AudioBook ab("AB1", "Dune", "Macmillan Audio", 2020, Money::of(15),
                 "https://audio.example/dune", "Scott Brick", 1260);
    assert(ab.category() == ResourceCategory::AudioBook);
    assert(ab.isDigital());
    assert(ab.narrator() == "Scott Brick");
    assert(ab.durationMinutes() == 1260);
    assert(ab.costFor(2) == Money::of(30));

    Thesis th("TH1", "Deep Learning for NLP", "IIIT Hyderabad", "Ph.D.",
              "Dr. P. Kumar", 2025);
    assert(th.category() == ResourceCategory::Thesis);
    assert(!th.isDigital());
    assert(th.unitPrice() == Money::of(0));
    assert(th.university() == "IIIT Hyderabad");
    assert(th.degree() == "Ph.D.");
    assert(th.supervisor() == "Dr. P. Kumar");
    assert(th.costFor(1) == Money::of(0));
}

static void testQ4HardcoverBooks() {
    Book pb("B_PB", "Paperback Book", {"Author"}, "isbn1", "Pub", 2020,
            Money::of(100), 1, Binding::Paperback);
    Book hc("B_HC", "Hardcover Book", {"Author"}, "isbn2", "Pub", 2020,
            Money::of(100), 1, Binding::Hardcover);

    // Paperback listed 100 -> costFor(1) is 100
    assert(pb.costFor(1) == Money::of(100));
    // Hardcover costs 20% more than listed price: 100 * 1.20 = 120
    assert(hc.costFor(1) == Money::of(120));
    assert(hc.costFor(3) == Money::of(360));
}

static void testQ5BulkDiscounts() {
    // Print items: 10% off for 10 or more copies
    Book pb("B1", "Book", {"A"}, "isbn", "P", 2020, Money::of(100));
    assert(pb.costFor(9) == Money::of(900));
    // 10 copies @ 100 with 10% off: 1000 - 100 = 900
    assert(pb.costFor(10) == Money::of(900));
    // 20 copies @ 100 with 10% off: 2000 - 200 = 1800
    assert(pb.costFor(20) == Money::of(1800));

    // Hardcover book: 20% more base price (120) and 10% off for 10 copies: 1200 - 120 = 1080
    Book hc("B2", "Hardcover Book", {"A"}, "isbn", "P", 2020, Money::of(100), 1, Binding::Hardcover);
    assert(hc.costFor(10) == Money::of(1080));

    // Electronic resources: seats beyond 50th cost half price
    ElectronicResource er("ER1", "E-Resource", "Pub", 2026, Money::of(10), "url",
                          LicenseModel::AnnualSubscription, Money::of(0));
    assert(er.costFor(50) == Money::of(500));
    // 60 seats: 50 * 10 + 10 * 5 = 550
    assert(er.costFor(60) == Money::of(550));
    // 100 seats: 50 * 10 + 50 * 5 = 750
    assert(er.costFor(100) == Money::of(750));
}

static void testQ6Taxes() {
    Catalog c;
    c.emplace<Book>("B1", "Print Book", std::vector<std::string>{"A"}, "isbn", "Pub", 2020, Money::of(100));
    c.emplace<ElectronicResource>("E1", "E-Res", "Pub", 2026, Money::of(50), "url");

    Budget b(Money::of(1000));
    AcquisitionManager acq(c, b);

    // Set 10% print tax, 20% electronic tax
    acq.setTaxRates(0.10, 0.20);
    assert(acq.quotePreTax("B1", 1) == Money::of(100));
    // Post tax: 100 + 10% = 110
    assert(acq.quote("B1", 1) == Money::of(110));

    assert(acq.quotePreTax("E1", 2) == Money::of(100));
    // Post tax: 100 + 20% = 120
    assert(acq.quote("E1", 2) == Money::of(120));

    // Purchase checks quota against post-tax cost
    b.setQuota(ResourceCategory::Book, {5, Money::of(105)});
    std::string why;
    // Pre-tax 100 fits 105, but post-tax 110 exceeds spend quota 105!
    assert(!acq.canPurchase("B1", 1, &why));
    assert(why.find("spend quota exceeded") != std::string::npos);
}

static void testQ7TitleLimit() {
    Budget b(Money::of(2000));
    // Quota: max 10 units, 2000 spend, max 2 distinct titles
    b.setQuota(ResourceCategory::Book, {10, Money::of(2000), 2});

    // Buying title B001: ok (1st title)
    assert(b.check(ResourceCategory::Book, 2, Money::of(200), "B001").empty());
    b.commit(ResourceCategory::Book, 2, Money::of(200), "B001");
    assert(*b.titlesRemaining(ResourceCategory::Book) == 1);

    // Buying more of B001: ok (same title, does not consume title limit)
    assert(b.check(ResourceCategory::Book, 2, Money::of(200), "B001").empty());
    b.commit(ResourceCategory::Book, 2, Money::of(200), "B001");
    assert(*b.titlesRemaining(ResourceCategory::Book) == 1);

    // Buying title B002: ok (2nd title)
    assert(b.check(ResourceCategory::Book, 1, Money::of(100), "B002").empty());
    b.commit(ResourceCategory::Book, 1, Money::of(100), "B002");
    assert(*b.titlesRemaining(ResourceCategory::Book) == 0);

    // Buying title B003: rejected (title limit reached)
    assert(!b.check(ResourceCategory::Book, 1, Money::of(100), "B003").empty());
    assertThrows<QuotaExceededError>([&]() {
        b.commit(ResourceCategory::Book, 1, Money::of(100), "B003");
    });
}

static void testQ8Cancellation() {
    Catalog c;
    c.emplace<Book>("B1", "C++", std::vector<std::string>{"Stroustrup"}, "isbn", "Pub", 2020, Money::of(100));
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(800)});
    AcquisitionManager acq(c, b);

    const auto& order = acq.purchase("B1", 3);
    int orderNum = order.orderNo;
    assert(b.spent() == Money::of(300));
    assert(c.holdings("B1") == 3);

    // Cancel order
    const auto& cancelRec = acq.cancelOrder(orderNum);
    assert(cancelRec.isCancellation);
    assert(b.spent() == Money::of(0));
    assert(c.holdings("B1") == 0);
    assert(*b.unitsRemaining(ResourceCategory::Book) == 10);
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(800));

    // History contains both original and cancellation records
    assert(acq.history().size() == 2);
    assert(acq.history()[0].orderNo == orderNum && !acq.history()[0].isCancellation);
    assert(acq.history()[1].isCancellation);

    // Cannot cancel again
    assertThrows<std::invalid_argument>([&]() { acq.cancelOrder(orderNum); });
}

static void testQ9DepartmentBudgets() {
    Catalog c;
    c.emplace<Book>("B_CS", "CS Book", std::vector<std::string>{"CS Author"}, "isbn1", "P", 2020, Money::of(150));
    c.emplace<Book>("B_PH", "Physics Book", std::vector<std::string>{"PH Author"}, "isbn2", "P", 2020, Money::of(200));

    Budget centralBudget(Money::of(5000));
    AcquisitionManager acq(c, centralBudget);

    Budget csBudget(Money::of(1000));
    csBudget.setQuota(ResourceCategory::Book, {4, Money::of(600)});
    acq.setDepartmentBudget("Computer Science", csBudget);

    Budget physBudget(Money::of(800));
    physBudget.setQuota(ResourceCategory::Book, {3, Money::of(500)});
    acq.setDepartmentBudget("Physics", physBudget);

    // Purchase charged to Computer Science
    acq.purchase("B_CS", 2, "Computer Science");
    assert(acq.departmentBudget("Computer Science")->spent() == Money::of(300));
    assert(centralBudget.spent() == Money::of(0)); // Central budget unaffected

    // Purchase charged to Physics
    acq.purchase("B_PH", 1, "Physics");
    assert(acq.departmentBudget("Physics")->spent() == Money::of(200));

    // Exceeding CS quota
    assertThrows<QuotaExceededError>([&]() {
        acq.purchase("B_CS", 3, "Computer Science"); // 2 + 3 = 5 > maxUnits 4
    });
}

static void testQ10RolloverAndWarnings() {
    Budget b(Money::of(10000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(5000)});
    b.commit(ResourceCategory::Book, 4, Money::of(2000));
    // Spent 2000, remaining = 8000
    // Rollover 25% of unspent amount + 5000 new allocation = 2000 + 5000 = 7000
    Budget nextYear = b.createNextYearBudget(25.0, Money::of(5000));
    assert(nextYear.total() == Money::of(7000));
    assert(nextYear.spent() == Money::of(0));
    assert(nextYear.quotaFor(ResourceCategory::Book)->maxUnits == 10);

    // Quota warning check at 80%
    b.commit(ResourceCategory::Book, 4, Money::of(2000)); // Total 8 units / 10 = 80%
    assert(b.isPastQuotaThreshold(ResourceCategory::Book, 0.80));
    assert(!b.quotaWarning(ResourceCategory::Book, 0.80).empty());
}

static void testQ11AllOrNothingBatch() {
    Catalog c;
    c.emplace<Book>("B1", "Book 1", std::vector<std::string>{"A"}, "isbn", "P", 2020, Money::of(100));
    c.emplace<Book>("B2", "Book 2", std::vector<std::string>{"A"}, "isbn", "P", 2020, Money::of(500));
    Budget b(Money::of(400)); // Budget only 400

    AcquisitionManager acq(c, b);

    // Batch: B1 (100) fits, but B2 (500) exceeds budget
    std::vector<PurchaseRequest> batch = {{"B1", 1}, {"B2", 1}};

    // With allOrNothing = true: nothing should be bought
    auto results = acq.processBatch(batch, true);
    assert(results.size() == 2);
    assert(!results[0].approved);
    assert(!results[1].approved);
    assert(b.spent() == Money::of(0));
    assert(c.holdings("B1") == 0);
    assert(c.holdings("B2") == 0);

    // Batch where all requests fit
    std::vector<PurchaseRequest> goodBatch = {{"B1", 1}, {"B1", 2}};
    auto goodResults = acq.processBatch(goodBatch, true);
    assert(goodResults.size() == 2);
    assert(goodResults[0].approved);
    assert(goodResults[1].approved);
    assert(b.spent() == Money::of(300));
    assert(c.holdings("B1") == 3);
}

static void testQ12Vendors() {
    Catalog c;
    auto& b = c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "isbn", "Pub", 2020, Money::of(100));
    // Add multiple vendor offers
    b.addVendor("Amazon", Money::of(85));
    b.addVendor("BookDepository", Money::of(75));
    b.addVendor("LocalBookstore", Money::of(90));

    // Cheapest vendor should be BookDepository at 75
    auto best = b.cheapestVendor();
    assert(best.has_value());
    assert(best->vendor == "BookDepository");
    assert(best->price == Money::of(75));
    assert(b.effectiveUnitPrice() == Money::of(75));

    Budget budget(Money::of(1000));
    AcquisitionManager acq(c, budget);
    const auto& rec = acq.purchase("B1", 2);
    assert(rec.cost == Money::of(150)); // 75 * 2
    assert(rec.vendor == "BookDepository"); // Recorded vendor
}

static void testQ13SearchesAndSaveLoad() {
    Catalog c;
    c.emplace<Book>("B1", "The C++ Programming Language", std::vector<std::string>{"Bjarne Stroustrup"},
                    "978-0321563842", "Addison-Wesley", 2013, Money::of(1200));
    c.emplace<Journal>("J1", "Communications of the ACM", "ACM", 2024, Money::of(200),
                       "0001-0782", 12, 1);
    c.emplace<EBook>("EB1", "Effective Modern C++", std::vector<std::string>{"Scott Meyers"},
                    "978-1491903995", "O'Reilly", 2014, Money::of(50), "url");

    // Search by author
    auto byAuthor = c.searchAuthor("Stroustrup");
    assert(byAuthor.size() == 1 && byAuthor[0]->id() == "B1");

    auto byMeyers = c.searchAuthor("meyers"); // case-insensitive
    assert(byMeyers.size() == 1 && byMeyers[0]->id() == "EB1");

    // Search by ISBN/ISSN
    auto byIsbn = c.searchIsbnOrIssn("978-0321563842");
    assert(byIsbn.size() == 1 && byIsbn[0]->id() == "B1");

    auto byIssn = c.searchIsbnOrIssn("0001-0782");
    assert(byIssn.size() == 1 && byIssn[0]->id() == "J1");

    // Search by year range
    auto byYear = c.searchYearRange(2013, 2020);
    assert(byYear.size() == 2); // B1 (2013) and EB1 (2014)

    // Save and load back
    c.addHoldings("B1", 5);
    std::stringstream ss;
    c.save(ss);

    Catalog loaded;
    loaded.load(ss);
    assert(loaded.size() == 3);
    assert(loaded.contains("B1") && loaded.contains("J1") && loaded.contains("EB1"));
    assert(loaded.holdings("B1") == 5);
    assert(loaded.get("J1").category() == ResourceCategory::Journal);
    assert(loaded.get("EB1").category() == ResourceCategory::EBook);
}

static void testQ14Lending() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "isbn", "Pub", 2020, Money::of(100));
    c.emplace<ElectronicResource>("ER1", "E-Res", "Pub", 2026, Money::of(50), "url");

    c.addHoldings("B1", 2);   // 2 print copies held
    c.addHoldings("ER1", 2);  // 2 digital seats licensed

    // Print borrowing
    assert(c.availableUnits("B1") == 2);
    c.borrowItem("B1");
    assert(c.activeLoans("B1") == 1);
    assert(c.availableUnits("B1") == 1);

    c.borrowItem("B1");
    assert(c.activeLoans("B1") == 2);
    assert(c.availableUnits("B1") == 0);

    // Exceeding available copies throws LibraryError
    assertThrows<LibraryError>([&]() { c.borrowItem("B1"); });

    // Returning copies
    c.returnItem("B1");
    assert(c.availableUnits("B1") == 1);

    // Digital sessions
    c.openSession("ER1");
    assert(c.activeLoans("ER1") == 1);
    c.openSession("ER1");
    assert(c.activeLoans("ER1") == 2);
    // Exceeding licensed seats throws LibraryError
    assertThrows<LibraryError>([&]() { c.openSession("ER1"); });

    c.closeSession("ER1");
    assert(c.activeLoans("ER1") == 1);

    // Mismatched loan calls throw invalid_argument
    assertThrows<std::invalid_argument>([&]() { c.borrowItem("ER1"); });
    assertThrows<std::invalid_argument>([&]() { c.openSession("B1"); });
}

int main() {
    std::cout << "Running test suite using standard C++ assert...\n";

    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();

    // Run new feature test suites
    testQ1Journal();
    testQ2EBook();
    testQ3AudioBookAndThesis();
    testQ4HardcoverBooks();
    testQ5BulkDiscounts();
    testQ6Taxes();
    testQ7TitleLimit();
    testQ8Cancellation();
    testQ9DepartmentBudgets();
    testQ10RolloverAndWarnings();
    testQ11AllOrNothingBatch();
    testQ12Vendors();
    testQ13SearchesAndSaveLoad();
    testQ14Lending();

    std::cout << "All tests passed successfully with standard assert!\n";
    return 0;
}
