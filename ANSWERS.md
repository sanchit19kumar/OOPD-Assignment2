# Assignment 2 — ANSWERS.md

**Course:** Object-Oriented Programming and Design — Monsoon 2026  
**Assignment:** Assignment 2 – Classes, Objects, Inheritance and Exception Handling  

---

## 1. Questions Attempted

All **16 questions** across all parts have been fully attempted and implemented:
- **Part A: Extending the class hierarchy**
  - Q1: `Journal` class (subscription pricing, ISSN, issues/yr, quota integration)
  - Q2: `EBook` class (inherited electronic pricing, format, DRM, written answers)
  - Q3: `AudioBook` and `Thesis` classes (base class selection & design justifications)
- **Part B: Pricing**
  - Q4: Hardcover books (+20% surcharge over unit price)
  - Q5: Bulk discounts (10% off for print $\ge 10$ copies; 50% off seats $> 50$ for electronic)
  - Q6: Configurable tax rates (print vs electronic), pre-tax and post-tax reporting & quota checking
- **Part C: Quotas and budget**
  - Q7: Limit on distinct titles per category
  - Q8: Order cancellation (refund budget & quotas, reduce holdings, history preservation)
  - Q9: Department budgets (department-specific spending limits & quotas)
  - Q10: Year-end rollover (configurable carry-forward) and 80% quota threshold warnings
- **Part D: Acquisition**
  - Q11: All-or-nothing batch processing option
  - Q12: Multiple vendors per title, automatic cheapest-vendor selection & order tracking
- **Part E: Catalogue**
  - Q13: Advanced queries (by author, ISBN/ISSN, publication year range) and Save/Load persistence
  - Q14: Patron lending (print copies borrow/return) and concurrent electronic user sessions
- **Part F: Money**
  - Q15: Currency code tagging; strict safety checks throwing `CurrencyMismatchError` on cross-currency arithmetic/comparisons
- **Part G: Code Cleanliness**
  - Q16: Test framework refactored to standard C++ `<cassert>` (`assert`) and template-based exception assertions, replacing macros

---

## 2. Written Answers to Specific Questions

### Question 2: Code Duplication between `Book` and `EBook`

**Question:** An e-book is also a kind of book. Which code is now duplicated between `Book` and `EBook`, and how could you avoid the duplication?

**Answer:**
1. **Identified Duplication:**
   - **Fields:** Both classes store `std::vector<std::string> authors_` and `std::string isbn_`.
   - **Accessors:** Both implement identical `authors()` and `isbn()` accessors.
   - **Search Helpers:** Both implement identical `searchAuthors()` and `searchIdentifier()` methods returning `authors_` and `isbn_`.
   - **Formatting & Printing:** Both format authors using `joinAuthors()` and print `"  authors: ..."` and `"  isbn: ..."` in `printDetails()`.

2. **How to Avoid the Duplication:**
   - **Composition (Recommended in C++):** Create a reusable `BookMetadata` or `BibliographicInfo` class/struct containing `authors`, `isbn`, edition, and helper formatting methods. Both `Book` and `EBook` compose a `BookMetadata` instance, delegating bibliographic queries and printing directly to this shared object.
   - **Multiple Inheritance / Mixin Interfaces:** Define an abstract `BookIdentity` interface providing author and ISBN management. `Book` inherits from `Resource` and `BookIdentity`, while `EBook` inherits from `ElectronicResource` and `BookIdentity`.
   - **Bridge Pattern / Decoupled Dimensions:** Recognize that "Content Type" (Book, Journal, Thesis) and "Delivery Medium" (Physical Print vs Digital Concurrent Seat) are orthogonal dimensions. Modeling content via one hierarchy and licensing/medium via a composed strategy cleanly avoids diamond inheritance and duplication.

---

### Question 3: Base Class Choices and Justifications

#### 1. `AudioBook` — Base Class: `ElectronicResource`
- **Justification:** In modern library science and digital collection management, an audiobook is delivered as a digital media stream or downloadable digital file accessed via a remote digital access URL. It is hosted on a publisher platform (incurring a platform fee), governed by concurrent digital user seat licensing models (such as annual subscriptions or perpetual licences), and reports `isDigital() == true`. Deriving `AudioBook` from `ElectronicResource` reuses the seat-based pricing model, URL tracking, platform fees, and digital licensing semantics without redundant code.

#### 2. `Thesis` — Base Class: `Resource`
- **Justification:** A university dissertation or thesis is an institutional academic monograph deposited directly in the university repository. It is typically a physical archival volume or open-access deposit. Critically, it is deposited without an acquisition cost (`unitPrice = Money::of(0)`), does not incur commercial platform hosting fees, does not have seat licensing tiers, and does not require subscription renewal cycles. Therefore, inheriting directly from the base `Resource` class correctly models its nature without carrying extraneous electronic licensing infrastructure.

---

## 3. Key Design Decisions

1. **Precision Currency and Multi-Currency Safety (Q15):**
   - Extended `Money` to hold a currency code (default `"INR"`).
   - In all arithmetic (`+`, `-`, `+=`, `-=`) and relational operators (`==`, `!=`, `<`, `<=`, `>`, `>=`), currencies are validated first; any mismatch throws `CurrencyMismatchError` (derived from `LibraryError`).

2. **Extensible Pricing Hierarchy (Q4, Q5):**
   - Hardcover pricing (+20%) is evaluated at the `Book` level before applying bulk discounts.
   - Print bulk discounts (10% off for $\ge 10$ copies) are implemented in `Resource::costFor` and overridden in `Book` and `Journal`.
   - Electronic bulk discounts (seats beyond 50 cost half price) are implemented in `ElectronicResource::costFor`, automatically covering all digital subclasses (`EBook`, `AudioBook`).

3. **Taxes and Quotas (Q6):**
   - Configurable print and electronic tax rates on `AcquisitionManager`.
   - Pre-tax and post-tax costs are preserved in each `PurchaseRecord`.
   - In accordance with Q6, quotas and remaining budget are strictly validated against **post-tax costs**.

4. **Title Limits per Category (Q7):**
   - `Quota` extended with `maxTitles` (-1 = unlimited).
   - `Budget` tracks unique title IDs per category (`titlesBought_`). Purchasing additional copies of an already-acquired title does not decrement the title quota; acquiring a new title enforces `maxTitles`.

5. **Order Cancellation & History (Q8):**
   - `cancelOrder(orderNo)` validates and reverses the budget spend and category unit/spend quotas.
   - Catalog holdings are decremented by the cancelled quantity.
   - The original purchase record is retained in history; a separate negative-cost cancellation record is appended with reason `"Cancelled order #..."`.

6. **Department Budgets (Q9):**
   - `AcquisitionManager` supports multiple department budgets mapped by department name.
   - `PurchaseRequest` includes an optional `department` field. When specified, orders are validated, committed, and refunded against the department's budget rather than the central budget.

7. **All-or-Nothing Batch Acquisition (Q11):**
   - `processBatch` accepts an `allOrNothing` boolean flag.
   - When enabled, a simulation phase checks all requests sequentially against simulated budgets. If any request fails (missing resource, invalid quantity, or exceeded quota/budget), nothing is committed and all records are marked rejected.

8. **Vendor Selection (Q12):**
   - Resources can maintain multiple `VendorOffer` entries.
   - `effectiveUnitPrice()` dynamically selects the cheapest vendor offer.
   - Order history records indicate the vendor used.

9. **Polymorphic Save and Load (Q13):**
   - Each derived resource implements `serialize()`.
   - `Catalog::load` parses the tokenized records and reconstructs the correct derived type (`Book`, `Journal`, `ElectronicResource`, `EBook`, `AudioBook`, `Thesis`) via a polymorphic factory mechanism.

10. **Lending System (Q14):**
    - `Catalog` tracks holdings, active loans, and available units.
    - Print borrowing (`borrowItem`/`returnItem`) is capped by physical copies held.
    - Digital sessions (`openSession`/`closeSession`) are capped by licensed concurrent seats.

11. **Standard Assertion Testing (Q16):**
    - Replaced the macro-based `CHECK` and `CHECK_THROWS` system with standard C++ `assert` from `<cassert>`.
    - Implemented a clean template function `assertThrows<ExceptionType>(lambda)` for robust exception verification.
