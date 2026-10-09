# BookManagement Assignment: Step-by-Step Question Explanation Guide

Welcome! This guide explains every question of **Assignment 2 (Monsoon 2026: Classes, Objects, Inheritance and Exception Handling)** in simple, clear, and beginner-friendly terms.

---

## Table of Contents
1. [Overview of the Project Architecture](#overview-of-the-project-architecture)
2. [Part A: Extending the Class Hierarchy (Q1, Q2, Q3)](#part-a-extending-the-class-hierarchy)
3. [Part B: Pricing (Q4, Q5, Q6)](#part-b-pricing)
4. [Part C: Quotas and Budget (Q7, Q8, Q9, Q10)](#part-c-quotas-and-budget)
5. [Part D: Acquisition (Q11, Q12)](#part-d-acquisition)
6. [Part E: Catalogue (Q13, Q14)](#part-e-catalogue)
7. [Part F: Money (Q15)](#part-f-money)
8. [Part G: Code Cleanliness (Q16)](#part-g-code-cleanliness)
9. [How to Build and Run](#how-to-build-and-run)

---

## Overview of the Project Architecture

The library system consists of four main building blocks:
1. **`Resource` Hierarchy:** Represents items in the library (`Book`, `Journal`, `EBook`, `ElectronicResource`, `AudioBook`, `Thesis`).
2. **`Money`:** An immutable-like value class that stores currency amounts in whole minor units (paise/cents) using `int64_t` so that rounding errors never occur.
3. **`Budget` & `Quota`:** Tracks university spending limits both overall and per category (e.g. max copies and max spend for books).
4. **`Catalog` & `AcquisitionManager`:** The `Catalog` owns the resources and tracks physical copies/digital seats. The `AcquisitionManager` checks quotes, verifies budget/quotas, approves orders, records history, and issues reports.

```
                    ┌─────────────────────────┐
                    │    Resource (Abstract)  │
                    └────────────┬────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         │                       │                       │
 ┌───────┴───────┐       ┌───────┴───────┐       ┌───────┴───────┐
 │     Book      │       │    Journal    │       │     Thesis    │
 └───────────────┘       └───────────────┘       └───────────────┘
                                 │
                    ┌────────────┴────────────┐
                    │    ElectronicResource   │
                    └────────────┬────────────┘
                                 │
                 ┌───────────────┴───────────────┐
                 │                               │
         ┌───────┴───────┐               ┌───────┴───────┐
         │     EBook     │               │   AudioBook   │
         └───────────────┘               └───────────────┘
```

---

## Part A: Extending the Class Hierarchy

### Q1. Journal
* **The Goal:** Model a physical academic journal that the library subscribes to on an annual basis.
* **Key Fields Added:**
  * `issn`: International Standard Serial Number (unique journal identifier).
  * `issuesPerYear`: Frequency of publication (e.g. 12 issues for a monthly journal).
  * `subscriptionYears`: Number of years subscribed (default: 1; must be $\ge 1$).
* **Pricing Formula:**
  $$\text{Cost} = \text{Annual Unit Price} \times \text{Number of Copies} \times \text{Subscription Years}$$
* **System Integration:**
  * Added `ResourceCategory::Journal` to the enum, `categoryName()` to print `"Journal"`, and included it in `kAllCategories` so budgets and reports track it.
* **Validation:** If `subscriptionYears < 1`, it immediately throws `std::invalid_argument`.

---

### Q2. EBook
* **The Goal:** Model an electronic book.
* **Inheritance:** Inherits from `ElectronicResource` because an e-book is a digital asset accessed online.
* **Key Fields Added:**
  * `authors`: List of authors (`std::vector<std::string>`).
  * `isbn`: Book identifier.
  * `format`: File format (`"PDF"`, `"EPUB"`, or `"HTML"`).
  * `isDrmProtected`: Boolean indicating if DRM encryption is active.
* **Pricing:** Automatically inherits `ElectronicResource` pricing:
  $$\text{Cost} = \text{Platform Fee} + (\text{Seat Price} \times \text{Seats})$$
* **The Written Question Answer (Code Duplication):**
  * *What is duplicated?* `Book` and `EBook` both store `authors_`, `isbn_`, have identical getters, and print author/ISBN strings.
  * *How to solve it?* Use **Composition**: extract a `BookMetadata` struct and embed it inside both `Book` and `EBook`, delegating author and ISBN operations to it.

---

### Q3. AudioBook and Thesis
* **AudioBook:**
  * **Base Class:** `ElectronicResource`.
  * **Reasoning:** In modern libraries, audiobooks are digital streams or downloads accessed through URLs with concurrent-user seat licences and platform fees.
  * **Fields:** `narrator`, `durationMinutes`.
* **Thesis:**
  * **Base Class:** `Resource`.
  * **Reasoning:** A university thesis/dissertation is an institutional academic monograph deposited in physical/archival format, usually free of charge (`unitPrice = 0`), without commercial platform fees or seat licensing.
  * **Fields:** `university`, `degree`, `supervisor`.

---

## Part B: Pricing

### Q4. Hardcover Books
* **The Goal:** A hardcover binding is more expensive to produce than paperback.
* **Rule:** If a book has `Binding::Hardcover`, its effective unit price increases by **20%**.
* **Calculation:**
  $$\text{Hardcover Price} = \text{Base Price} \times 1.20 = \frac{\text{Base Price} \times 120}{100}$$
* **Implementation:** Handled inside `Book::costFor(quantity)`.

---

### Q5. Bulk Discounts
* **The Goal:** Reward bulk purchasing with tiered volume discounts.
* **Rule 1 (Print Items):** If buying **10 or more** copies of a print resource (`Book`, `Journal`), apply a **10% discount** on the total cost.
* **Rule 2 (Electronic Resources):** For digital items (`ElectronicResource`, `EBook`, `AudioBook`), the first 50 concurrent seats cost full price; every seat **beyond the 50th** is **50% off (half price)**.
  $$\text{Cost} = \text{Platform Fee} + (\text{Price} \times 50) + \left(\frac{\text{Price}}{2} \times (\text{Seats} - 50)\right)$$

---

### Q6. Taxes
* **The Goal:** Real-world purchases incur sales tax / VAT, and print items vs digital items are taxed at different rates.
* **Features:**
  * Configurable print tax rate (e.g. 5%) and electronic tax rate (e.g. 10%) on `AcquisitionManager`.
  * Order reports display both **pre-tax** and **post-tax** totals.
  * **Crucial Rule:** Quotas and overall budget limits are verified and committed against the **post-tax cost**.

---

## Part C: Quotas and Budget

### Q7. Title Limit per Category
* **The Goal:** Prevent a library department from buying 100 copies of only one book, or spending their entire quota on too many different book titles.
* **Implementation:** `Quota` now includes an optional `maxTitles` field.
* **Logic:** The budget tracks distinct title IDs purchased in each category. Buying more copies of an already-purchased book title is allowed, but attempting to buy a new (unseen) title when the title limit is reached throws `QuotaExceededError`.

---

### Q8. Order Cancellation
* **The Goal:** If a purchase was approved by mistake, cancel it cleanly.
* **Workflow:**
  1. Call `acq.cancelOrder(orderNo)`.
  2. The budget is refunded: spent amount and category quota usages are decremented.
  3. Catalog holdings are reduced by the order's quantity.
  4. The original approved record remains in the order history, and a separate cancellation record (with negative cost and explanation) is appended to maintain a complete audit trail.

---

### Q9. Department Budgets
* **The Goal:** Different academic departments (Computer Science, Physics, Chemistry, etc.) have separate financial accounts.
* **Solution:** `AcquisitionManager::setDepartmentBudget(name, budget)`.
* **Execution:** A `PurchaseRequest` can specify a `department`. If specified, the charge and quotas apply strictly to that department's budget without consuming central university funds.

---

### Q10. Year-End Rollover & Quota Warnings
* **Year-End Rollover:**
  * At fiscal year-end, university policies often allow carrying forward unspent budget.
  * `createNextYearBudget(carryForwardPercent, newBaseAllocation)` computes:
    $$\text{Rollover Amount} = \text{Remaining Budget} \times \frac{\text{Percentage}}{100}$$
    $$\text{Next Year Budget} = \text{New Allocation} + \text{Rollover Amount}$$
* **80% Quota Warning:**
  * When any category reaches $\ge 80\%$ of its unit quota or spend quota, an automated warning message is generated and logged to prevent unexpected overspending.

---

## Part D: Acquisition

### Q11. All-or-Nothing Batch Processing
* **The Goal:** When placing a purchase order containing multiple books, either all books are successfully purchased or none are bought at all (transactional consistency).
* **Solution:** Pass `allOrNothing = true` to `acq.processBatch(requests, true)`.
* **Mechanism:** A two-phase execution:
  1. **Phase 1 (Simulation):** Checks every request in order against a temporary simulation state.
  2. **Phase 2 (Commit):** If *any* request would fail (due to budget, quota, or missing ID), the batch is aborted, 0 funds are spent, and no holdings change. If all pass, all orders are executed.

---

### Q12. Vendors
* **The Goal:** The same title may be sold by different vendors (Amazon, BookDepository, Local Bookstore) at varying prices.
* **Solution:**
  * A resource stores a list of `VendorOffer(vendor, price)`.
  * `Resource::cheapestVendor()` finds the minimum price offer.
  * Purchases automatically execute at the cheapest vendor price, and the vendor name is stamped on the order record.

---

## Part E: Catalogue

### Q13. Searches and Save/Load Persistence
* **Searches:**
  * `searchAuthor("name")`: Case-insensitive search through author lists of books and e-books.
  * `searchIsbnOrIssn("code")`: Direct matching on ISBN (books/e-books) or ISSN (journals).
  * `searchYearRange(start, end)`: Matches resources published within the specified year range.
* **Save / Load:**
  * `saveToFile(filename)` writes each resource to a line formatted with its type tag and attributes.
  * `loadFromFile(filename)` parses each line and uses a polymorphic factory to instantiate the exact derived class (`Book`, `Journal`, `EBook`, `ElectronicResource`, `AudioBook`, `Thesis`).

---

### Q14. Patron Lending and Digital Sessions
* **Print Items:**
  * `borrowItem(id)`: Decrements available physical copies. If all copies are on loan, throws `LibraryError`.
  * `returnItem(id)`: Returns the copy, making it available again.
* **Digital Items:**
  * `openSession(id)`: Opens a concurrent user seat. If all licensed seats are active, throws `LibraryError`.
  * `closeSession(id)`: Closes the user session.

---

## Part F: Money

### Q15. Multi-Currency Money
* **The Goal:** Prevent dangerous financial bugs where amounts in USD and INR are accidentally added together without conversion.
* **Implementation:**
  * `Money` now carries a `currency` code (e.g. `"INR"`, `"USD"`, `"EUR"`).
  * Operators `+`, `-`, `+=`, `-=`, `==`, `!=`, `<`, `<=`, `>`, `>=` compare currency codes first.
  * If currencies differ, a `CurrencyMismatchError` (derived from `LibraryError`) is thrown.

---

## Part G: Code Cleanliness

### Q16. Replacing Macros with Standard `assert`
* **The Goal:** Preprocessor macros (`#define CHECK(...)`) can cause subtle scoping bugs and make debugging difficult.
* **Refactoring:**
  * Removed custom macros from `test_main.cpp`.
  * Used standard C++ `assert(condition)` from `<cassert>`.
  * Created a type-safe template function `assertThrows<ExceptionType>(lambda)` to verify exceptions cleanly:
    ```cpp
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
        assert(caught && "Expected exception was not thrown");
    }
    ```

---

## How to Build and Run

### 1. Running Tests
```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude src/*.cpp tests/test_main.cpp -o tests.exe
.\tests.exe
```

### 2. Running the Extended Feature Demo
```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude src/*.cpp examples/demo.cpp -o demo.exe
.\demo.exe
```
