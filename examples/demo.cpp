// Comprehensive Demo: demonstrates every feature of the extended BookManagement library
// Covering Part A through Part G (Q1 - Q16).

#include <iostream>
#include <sstream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    std::cout << "============================================================\n";
    std::cout << "      BookManagement Extended Demonstration (Q1 - Q16)      \n";
    std::cout << "============================================================\n\n";

    // 1. Class Hierarchy (Part A: Q1, Q2, Q3)
    std::cout << "--- 1. Class Hierarchy & Catalogue Setup ---\n";
    Catalog catalog;

    // Q1: Journal
    catalog.emplace<Journal>("J001", "IEEE Transactions on Software Engineering", "IEEE", 2024,
                            Money::of(400), "0098-5589", 12, 1);

    // Q2: EBook
    catalog.emplace<EBook>("EB001", "Designing Data-Intensive Applications",
                           std::vector<std::string>{"Martin Kleppmann"}, "978-1449373320",
                           "O'Reilly", 2017, Money::of(45), "https://oreilly.example/ddia",
                           "EPUB", true, LicenseModel::Perpetual, Money::of(500));

    // Q3: AudioBook & Thesis
    catalog.emplace<AudioBook>("AB001", "The Pragmatic Programmer (Audio)", "Addison-Wesley",
                              2020, Money::of(30), "https://audio.example/pragmatic",
                              "Anna Fields", 580);
    catalog.emplace<Thesis>("TH001", "Scalable Consensus in Distributed Ledgers",
                           "IIIT Hyderabad", "Ph.D.", "Dr. K. Sharma", 2025);

    // Standard Book & Hardcover (Q4)
    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(500), 1, Binding::Paperback);
    auto& b2 = catalog.emplace<Book>("B002", "The C++ Programming Language",
                                     std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                                     "Addison-Wesley", 2013, Money::of(1000), 4, Binding::Hardcover);

    // Q12: Add Vendor Offers to B002
    b2.addVendor("VendorAlpha", Money::of(950));
    b2.addVendor("VendorBeta", Money::of(850));  // Cheapest vendor

    for (const Resource* r : catalog.all()) {
        std::cout << "  " << r->summary() << "\n";
    }

    // 2. Pricing, Hardcover and Bulk Discounts (Part B: Q4, Q5)
    std::cout << "\n--- 2. Pricing: Hardcover, Bulk Discounts & Vendors ---\n";
    // Hardcover costs +20% on effective unit price (VendorBeta listed 850 -> 850 + 20% = 1020)
    std::cout << "  B002 Hardcover base effective price from cheapest vendor: " << b2.effectiveUnitPrice() << "\n";
    std::cout << "  B002 (Hardcover) 1 copy cost  = " << b2.costFor(1) << " (includes 20% hardcover surcharge)\n";
    std::cout << "  B002 (Hardcover) 10 copies     = " << b2.costFor(10) << " (includes 10% bulk discount)\n";

    // Electronic resource bulk discount: seats beyond 50th cost half price
    ElectronicResource eRes("R001", "ACM Digital Library", "ACM", 2026, Money::of(20),
                            "https://dl.acm.org", LicenseModel::AnnualSubscription, Money::of(1000));
    std::cout << "  R001 50 seats = " << eRes.costFor(50) << " (platform fee 1000 + 50*20)\n";
    std::cout << "  R001 70 seats = " << eRes.costFor(70) << " (50 seats @ 20 + 20 seats @ half price 10)\n";

    // 3. Budgets, Title Limits, and Warnings (Part C: Q7, Q10)
    std::cout << "\n--- 3. Budget, Quotas with Title Limits (Q7) & Warnings (Q10) ---\n";
    Budget budget(Money::of(30000));
    // Category Book: max 15 units, spend 12000, max 2 distinct titles
    budget.setQuota(ResourceCategory::Book, {15, Money::of(12000), 2});
    budget.setQuota(ResourceCategory::Journal, {10, Money::of(5000), 5});
    budget.setQuota(ResourceCategory::EBook, {50, Money::of(8000)});

    AcquisitionManager acq(catalog, budget);

    // Q6: Configure tax rates (5% print tax, 10% electronic tax)
    acq.setTaxRates(0.05, 0.10);
    std::cout << "  Configured tax rates: Print = 5%, Electronic = 10%\n";
    std::cout << "  Quote for J001 (1 copy): Pre-tax = " << acq.quotePreTax("J001", 1)
              << ", Post-tax = " << acq.quote("J001", 1) << "\n";

    // 4. Department Budgets (Q9)
    std::cout << "\n--- 4. Department Budgets (Q9) ---\n";
    Budget csBudget(Money::of(8000));
    csBudget.setQuota(ResourceCategory::Book, {10, Money::of(5000)});
    acq.setDepartmentBudget("Computer Science", csBudget);
    std::cout << "  Created dedicated budget for Department of Computer Science (Total: 8000)\n";

    // 5. Purchases & All-Or-Nothing Batch (Part D: Q11, Q12)
    std::cout << "\n--- 5. Purchases and All-Or-Nothing Batch (Q11) ---\n";
    // Normal purchases
    acq.purchase("J001", 2);
    acq.purchase("B001", 3);
    // Purchase charged to Computer Science department
    acq.purchase("B002", 2, "Computer Science");

    // All-or-nothing batch demo
    std::vector<PurchaseRequest> batch = {
        {"EB001", 5, ""},
        {"B001", 100, ""} // 100 copies will exceed quota/budget!
    };
    std::cout << "  Executing batch with allOrNothing = true (one request exceeds budget)...\n";
    auto batchResults = acq.processBatch(batch, true);
    std::cout << "  Batch result: 1st approved? " << (batchResults[0].approved ? "YES" : "NO")
              << ", 2nd approved? " << (batchResults[1].approved ? "YES" : "NO") << "\n";
    std::cout << "  Notice that because of allOrNothing, nothing from the batch was purchased!\n";

    // 6. Cancellation Demo (Q8)
    std::cout << "\n--- 6. Order Cancellation (Q8) ---\n";
    const auto& orderToCancel = acq.purchase("B001", 2);
    int cancelNum = orderToCancel.orderNo;
    std::cout << "  Purchased 2 copies of B001 in order #" << cancelNum
              << ". Holdings for B001 = " << catalog.holdings("B001") << "\n";
    std::cout << "  Cancelling order #" << cancelNum << "...\n";
    acq.cancelOrder(cancelNum);
    std::cout << "  Order cancelled. Holdings for B001 restored to: "
              << catalog.holdings("B001") << "\n";

    // 7. Year-End Rollover (Q10)
    std::cout << "\n--- 7. Year-End Budget Rollover (Q10) ---\n";
    std::cout << "  Current budget remaining = " << budget.remaining() << "\n";
    Budget nextYearBudget = budget.createNextYearBudget(30.0, Money::of(10000));
    std::cout << "  Rolled over 30% of unspent funds + 10,000 new allocation. Next year total: "
              << nextYearBudget.total() << "\n";

    // 8. Lending & Electronic Sessions (Part E: Q14)
    std::cout << "\n--- 8. Patron Lending & Electronic Sessions (Q14) ---\n";
    std::cout << "  B001 holdings: " << catalog.holdings("B001")
              << ", available for loan: " << catalog.availableUnits("B001") << "\n";
    catalog.borrowItem("B001");
    std::cout << "  Patron borrowed 1 copy of B001. Available now: "
              << catalog.availableUnits("B001") << "\n";
    catalog.returnItem("B001");
    std::cout << "  Patron returned B001. Available now: "
              << catalog.availableUnits("B001") << "\n";

    // 9. Searches and Save/Load (Q13)
    std::cout << "\n--- 9. Catalogue Searches and Save/Load (Q13) ---\n";
    auto searchRes = catalog.searchAuthor("Stroustrup");
    std::cout << "  Search author 'Stroustrup' found: " << searchRes.size() << " items ("
              << searchRes[0]->title() << ")\n";

    std::stringstream catalogStream;
    catalog.save(catalogStream);
    Catalog restoredCatalog;
    restoredCatalog.load(catalogStream);
    std::cout << "  Saved catalog and reloaded into fresh Catalog object: "
              << restoredCatalog.size() << " resources loaded successfully!\n";

    // 10. Multi-Currency Money (Part F: Q15)
    std::cout << "\n--- 10. Multi-Currency Safety (Q15) ---\n";
    Money usd = Money::of(100, 0, "USD");
    Money inr = Money::of(8000, 0, "INR");
    std::cout << "  Created USD: " << usd << " " << usd.currency()
              << " and INR: " << inr << " " << inr.currency() << "\n";
    try {
        Money sum = usd + inr;
        (void)sum;
    } catch (const CurrencyMismatchError& e) {
        std::cout << "  Caught expected CurrencyMismatchError: " << e.what() << "\n";
    }

    // Acquisition Report
    std::cout << "\n--- Acquisition Report (showing Pre-tax & Post-tax Totals) ---\n";
    acq.printReport(std::cout);

    std::cout << "\n============================================================\n";
    std::cout << "             All Demonstrations Completed!                  \n";
    std::cout << "============================================================\n";
    return 0;
}
