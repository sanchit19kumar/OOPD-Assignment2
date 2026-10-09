# Engineering Prompts Guide: Building the BookManagement System from Scratch

This guide contains a curated, battle-tested collection of **high-quality engineering prompts** designed to build, test, refactor, and submit the **BookManagement (C++17)** system step-by-step. 

Use these prompts with an AI pair-programmer or coding assistant to construct the project cleanly from starter code to final submission.

---

## Table of Contents
1. [Stage 0: Architecture & Requirement Ingestion](#stage-0-architecture--requirement-ingestion)
2. [Stage 1: Class Hierarchy Extension (Part A: Q1 - Q3)](#stage-1-class-hierarchy-extension-part-a-q1---q3)
3. [Stage 2: Pricing, Bulk Discounts & Taxes (Part B: Q4 - Q6)](#stage-2-pricing-bulk-discounts--taxes-part-b-q4---q6)
4. [Stage 3: Budgets, Quotas & Order Lifecycles (Part C: Q7 - Q10)](#stage-3-budgets-quotas--order-lifecycles-part-c-q7---q10)
5. [Stage 4: Acquisitions, Batch Transactions & Vendors (Part D: Q11 - Q12)](#stage-4-acquisitions-batch-transactions--vendors-part-d-q11---q12)
6. [Stage 5: Catalogue Persistence & Lending Engine (Part E: Q13 - Q14)](#stage-5-catalogue-persistence--lending-engine-part-e-q13---q14)
7. [Stage 6: Currency Safety & Financial Value Objects (Part F: Q15)](#stage-6-currency-safety--financial-value-objects-part-f-q15)
8. [Stage 7: Test Suite Refactoring & Standard Assertions (Part G: Q16)](#stage-7-test-suite-refactoring--standard-assertions-part-g-q16)
9. [Stage 8: Interactive Demonstration & Documentation (`demo.cpp` & `ANSWERS.md`)](#stage-8-interactive-demonstration--documentation-democpp--answersmd)
10. [Stage 9: Git History, Multi-Stage Commits & Packaging](#stage-9-git-history-multi-stage-commits--packaging)

---

## Stage 0: Architecture & Requirement Ingestion

### Prompt 0.1: Initial Codebase Scan & Extension Point Mapping
```text
I am starting an assignment to extend a C++17 library called BookManagement. 
Here is my starter codebase including Resource.h, Book.h, ElectronicResource.h, Budget.h, Catalog.h, Acquisition.h, and Money.h.
Please perform a detailed scan and identify:
1. All marked EXTENSION POINT comments across the header and implementation files.
2. The core responsibilities and ownership relationships between Catalog, Resource, Budget, and AcquisitionManager.
3. The compiler flags required (-std=c++17 -Wall -Wextra) and ensure no existing public API functions are broken.
Give me a concise dependency map before we write any code.
```

---

## Stage 1: Class Hierarchy Extension (Part A: Q1 - Q3)

### Prompt 1.1: Journal Class Implementation (Q1)
```text
Implement the Journal class derived from Resource in C++17:
- Header: include/bookmgmt/Journal.h, Source: src/Journal.cpp
- Fields: ISSN (std::string), issues per year (int, must be > 0), and subscription length in years (int, default 1, must be >= 1, otherwise throw std::invalid_argument).
- Cost calculation: unitPrice is the annual subscription price for one copy. costFor(copies) = unitPrice * copies * subscriptionYears.
- Hierarchy registration: Add ResourceCategory::Journal to ResourceCategory enum, update categoryName(), and add it to kAllCategories in Budget.cpp.
- Override printDetails(std::ostream&) to display the new fields.
Ensure full const-correctness and zero compiler warnings with -Wall -Wextra.
```

### Prompt 1.2: EBook Class Implementation & Written Answer (Q2)
```text
Implement the EBook class derived from ElectronicResource:
- Header: include/bookmgmt/EBook.h, Source: src/EBook.cpp
- Fields: authors (std::vector<std::string>), ISBN (std::string), file format ("PDF", "EPUB", or "HTML" - validate and throw std::invalid_argument on invalid values), and isDrmProtected (bool).
- Pricing: Inherit unchanged from ElectronicResource.
- Category: Register ResourceCategory::EBook.
- Output: printDetails() must call the base class's ElectronicResource::printDetails(os) followed by the new fields.
- Design Documentation: Add an in-depth comment at the top of EBook.h answering:
  "An e-book is also a kind of book. Which code is now duplicated between Book and EBook, and how could you avoid the duplication?"
  Cover composition (e.g. BookMetadata), mixin interfaces, and the Bridge pattern.
```

### Prompt 1.3: AudioBook & Thesis Classes with Inheritance Justification (Q3)
```text
Implement AudioBook and Thesis classes with architectural justifications:
1. AudioBook (derived from ElectronicResource):
   - Fields: narrator (std::string), duration in minutes (int, > 0).
   - Register ResourceCategory::AudioBook.
   - Justify in header comment why ElectronicResource is the correct base class (digital streaming/download delivery, concurrent seat licensing, platform fees).
2. Thesis (derived from Resource):
   - Fields: university (std::string), degree (std::string), supervisor (std::string).
   - Default unit price: Money::of(0) (deposited free of charge).
   - Register ResourceCategory::Thesis.
   - Justify in header comment why Resource is the correct base class (archival monograph, no platform fee, no seat license).
```

---

## Stage 2: Pricing, Bulk Discounts & Taxes (Part B: Q4 - Q6)

### Prompt 2.1: Hardcover Surcharge & Bulk Discount Tiers (Q4, Q5)
```text
Update the pricing engine in Resource.cpp, Book.cpp, and ElectronicResource.cpp:
1. Hardcover Surcharge (Q4): In Book::costFor, if binding == Binding::Hardcover, add a 20% surcharge to the base unit price before computing total copy costs.
2. Bulk Discounts (Q5):
   - Physical/Print items: When 10 or more copies are purchased at once, apply a 10% discount to the total cost. Apply this in Book::costFor, Journal::costFor, and base Resource::costFor.
   - Electronic resources: In ElectronicResource::costFor, every seat beyond the 50th costs half price (unitPrice / 2). The base platformFee remains unaffected.
Use integer minor units arithmetic (paise/cents) to prevent floating-point rounding inaccuracies.
```

### Prompt 2.2: Configurable Taxes & Pre/Post-Tax Reporting (Q6)
```text
Extend AcquisitionManager to support taxation:
1. Configuration: Add setTaxRates(double printRate, double electronicRate) with getters. Validate non-negative rates.
2. Quote Calculation: Add quotePreTax(id, quantity) returning pre-tax cost, and calculatePostTax(resource, preTaxCost) that rounds minor units accurately.
3. Order History: In PurchaseRecord, store both preTaxCost and postTaxCost.
4. Quota Enforcement: Crucial requirement - check category spend quota and overall budget against POST-TAX costs in canPurchase, purchase, and processBatch.
5. Report: Update printReport(std::ostream&) to display columns for both pre-tax and post-tax amounts, along with total pre-tax and post-tax spending.
```

---

## Stage 3: Budgets, Quotas & Order Lifecycles (Part C: Q7 - Q10)

### Prompt 3.1: Distinct Title Quotas per Category (Q7)
```text
Implement distinct title limits in Budget and Quota:
1. Extend struct Quota with `int maxTitles = -1;` (-1 means unlimited).
2. In class Budget, track unique purchased title IDs per category using `std::map<ResourceCategory, std::set<std::string>> titlesBought_`.
3. In Budget::check and Budget::commit:
   - If a purchase request is for a title ID already in titlesBought_[c], approve without consuming a title quota slot.
   - If it is a new title ID, ensure titlesBought_[c].size() + 1 does not exceed maxTitles; otherwise reject with a descriptive message or throw QuotaExceededError.
4. Add titlesRemaining(c) and update Budget::print() to show "Titles used/max".
```

### Prompt 3.2: Order Cancellation & Inventory Reversal (Q8)
```text
Implement order cancellation in AcquisitionManager and Budget:
1. In AcquisitionManager, implement `const PurchaseRecord& cancelOrder(int orderNo)`.
2. Validation: Ensure order exists, was approved, and is not already a cancellation record; throw std::invalid_argument otherwise.
3. State Reversal:
   - Call `Budget::refund(category, quantity, postTaxCost, titleId)` to restore the spent money and unit quota usage.
   - Call `Catalog::addHoldings(resourceId, -quantity)` to decrement inventory.
4. Audit Trail: Keep the original order record unmodified in history, and append a new record marked `isCancellation = true` with negative quantity and negative costs, with reason "Cancelled order #X".
```

### Prompt 3.3: Departmental Budgets & Multi-Tenant Accounting (Q9)
```text
Add departmental accounting to AcquisitionManager:
1. Allow registering department-specific budgets: `setDepartmentBudget(const std::string& dept, Budget b)`.
2. Extend `PurchaseRequest` with an optional `std::string department = "";`.
3. Update `canPurchase`, `purchase`, and `processBatch`:
   - If department is empty, validate and commit against the main institutional budget.
   - If department is specified, route checks, commits, and refunds to the department's dedicated Budget instance. Throw NotFoundError if department does not exist.
4. Record the department in `PurchaseRecord` and display it in the acquisition report.
```

### Prompt 3.4: Year-End Budget Rollover & 80% Threshold Warnings (Q10)
```text
Implement rollover and proactive quota warnings in Budget:
1. Year-End Rollover: Implement `Budget createNextYearBudget(double carryForwardPercent, Money newBaseAllocation) const`:
   - Calculate unspent funds = total_ - spent_.
   - Carry forward the specified percentage (0 to 100%) and add it to newBaseAllocation to create the new budget total.
   - Carry forward all existing category quota rules into the new Budget instance.
2. 80% Threshold Warnings:
   - Implement `isPastQuotaThreshold(category, threshold = 0.80)` and `quotaWarning(category, threshold = 0.80)`.
   - In `Budget::commit`, if units used / maxUnits >= 80% or spend used / maxSpend >= 80%, output a clear warning message to `std::clog`.
```

---

## Stage 4: Acquisitions, Batch Transactions & Vendors (Part D: Q11 - Q12)

### Prompt 4.1: All-or-Nothing Batch Transaction Processing (Q11)
```text
Enhance AcquisitionManager::processBatch with an all-or-nothing transactional mode:
- Signature: `std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest>& reqs, bool allOrNothing = false);`
- When `allOrNothing == true`:
  1. Execute a dry-run simulation phase against copies of the relevant budgets.
  2. If ANY item in the batch fails (resource not found, non-positive quantity, unit quota exceeded, spend quota exceeded, or budget exceeded), reject the ENTIRE batch.
  3. Ensure no money is committed, no holdings are changed, and all items in the returned list are marked `approved = false` with appropriate reasons.
  4. Only commit changes if all requests in the batch succeed.
```

### Prompt 4.2: Vendor Market Comparison & Cheapest Vendor Selection (Q12)
```text
Implement multi-vendor price comparison:
1. In Resource.h, add `struct VendorOffer { std::string vendor; Money price; };`.
2. Add `addVendor(vendor, price)`, `vendors()`, `cheapestVendor()`, and `effectiveUnitPrice()`.
   - `effectiveUnitPrice()` should return the cheapest vendor price if vendors are present; otherwise return the catalog unitPrice.
3. In `AcquisitionManager::purchase` and `processBatch`:
   - Automatically purchase from the cheapest vendor.
   - Record the chosen vendor name in `PurchaseRecord::vendor`.
   - Display the vendor in the acquisition report.
```

---

## Stage 5: Catalogue Persistence & Lending Engine (Part E: Q13 - Q14)

### Prompt 5.1: Multi-Criteria Catalogue Queries & Polymorphic File Serialization (Q13)
```text
Extend Catalog in Catalog.h and Catalog.cpp:
1. Search Capabilities:
   - `searchAuthor(author)`: Case-insensitive search across author lists (Book, EBook).
   - `searchIsbnOrIssn(code)`: Match ISBN (Book, EBook) or ISSN (Journal).
   - `searchYearRange(startYear, endYear)`: Filter by publication year inclusive.
2. File Persistence:
   - Add `virtual std::string serialize() const = 0` to Resource and implement it in all derived classes (Book, Journal, ElectronicResource, EBook, AudioBook, Thesis).
   - Implement `save(std::ostream&)` and `saveToFile(filename)`.
   - Implement `load(std::istream&)` and `loadFromFile(filename)`: Parse serialized lines, detect the type tag ("Book", "Journal", etc.), and dynamically construct the exact derived object with its holdings using `emplace<T>`.
```

### Prompt 5.2: Circulation Desk & Concurrent Digital Seat Sessions (Q14)
```text
Implement circulation and access control in Catalog:
1. Internal State: Track `holdings` (total owned) and `activeLoans` (in-use copies or sessions) per resource entry.
2. Physical Print Circulation:
   - `borrowItem(id)`: Increments activeLoans. Throws `LibraryError` if activeLoans >= holdings. Rejects digital items.
   - `returnItem(id)`: Decrements activeLoans. Throws if activeLoans <= 0.
3. Electronic Concurrent Access:
   - `openSession(id)`: Increments active session count. Throws `LibraryError` if activeLoans >= licensed seats (holdings). Rejects physical items.
   - `closeSession(id)`: Decrements session count. Throws if activeLoans <= 0.
4. Add `availableUnits(id)` returning `holdings - activeLoans`.
```

---

## Stage 6: Currency Safety & Financial Value Objects (Part F: Q15)

### Prompt 6.1: Multi-Currency Tagging & Type-Safe Currency Operations (Q15)
```text
Harden the Money class with currency safety:
1. Data Members: Add `std::string currency_ = "INR"` to Money.
2. Constructors & Factories: Update `Money::of` and `Money::fromMinor` to accept an optional currency code.
3. Currency Checking:
   - Add a private helper `void checkSameCurrency(const Money& o) const`.
   - If currencies differ, throw `CurrencyMismatchError(currency_, o.currency_)` (derived from LibraryError).
4. Operator Safety:
   - Call `checkSameCurrency` in `operator+=`, `operator-=`, `operator+`, `operator-`.
   - Call `checkSameCurrency` in all comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`.
5. Ensure scalar multiplication (`operator*`) retains the current currency without throwing.
```

---

## Stage 7: Test Suite Refactoring & Standard Assertions (Part G: Q16)

### Prompt 7.1: Replace Non-Standard Macros with `<cassert>` & Exception Assertions (Q16)
```text
Refactor tests/test_main.cpp to eliminate macro-based test frameworks:
1. Include `<cassert>` and remove all custom test macros (`CHECK`, `CHECK_THROWS`, `REQUIRE`).
2. Replace simple assertions with standard `assert(condition)`.
3. Exception Testing: Implement a clean template function:
   ```cpp
   template <typename ExceptionType, typename Func>
   void assertThrows(Func&& fn) {
       bool caught = false;
       try { fn(); }
       catch (const ExceptionType&) { caught = true; }
       catch (...) { caught = false; }
       assert(caught && "Expected exception was not thrown");
   }
   ```
4. Write dedicated test functions for every single assignment feature:
   - `testQ1Journal`, `testQ2EBook`, `testQ3AudioBookAndThesis`
   - `testQ4HardcoverBooks`, `testQ5BulkDiscounts`, `testQ6Taxes`
   - `testQ7TitleLimit`, `testQ8Cancellation`, `testQ9DepartmentBudgets`, `testQ10RolloverAndWarnings`
   - `testQ11AllOrNothingBatch`, `testQ12Vendors`, `testQ13SearchesAndSaveLoad`, `testQ14Lending`, `testMoney` (Q15).
5. Ensure the suite compiles without warnings under `-Wall -Wextra` and outputs a clear success message.
```

---

## Stage 8: Interactive Demonstration & Documentation (`demo.cpp` & `ANSWERS.md`)

### Prompt 8.1: Full-System Demonstration Script (`examples/demo.cpp`)
```text
Create a demonstration program in examples/demo.cpp that demonstrates every feature:
1. Setup catalog with all resource types (Book, Journal, EBook, AudioBook, Thesis).
2. Show vendor comparison, hardcover surcharge (+20%), and print/electronic bulk discounts.
3. Configure print & electronic taxes, display pre-tax and post-tax quotes.
4. Setup departmental budgets and show purchases charged to specific departments.
5. Demonstrate all-or-nothing batch purchases when a quota is exceeded vs when all succeed.
6. Purchase an item, display holdings, cancel the order, and show inventory & quota refund.
7. Demonstrate 80% quota warning triggers and year-end budget rollover with carry-forward funds.
8. Demonstrate patron borrowing/returning and electronic concurrent session limits.
9. Demonstrate author search, save the catalog to stream, and restore it into a new Catalog object.
10. Demonstrate currency safety catching CurrencyMismatchError.
11. Print the complete order history report displaying pre-tax and post-tax totals.
```

### Prompt 8.2: Comprehensive Submission Documentation (`ANSWERS.md`)
```text
Generate ANSWERS.md fulfilling all submission requirements:
1. List of Attempted Questions: Explicitly confirm all 16 questions (Q1 to Q16 across Parts A-G) are attempted.
2. Question 2 Written Answer:
   - Detail the code duplicated between Book and EBook (authors, ISBN, search helpers, formatting).
   - Explain how to avoid the duplication using Composition (BookMetadata), Mixin interfaces, and the Bridge pattern.
3. Question 3 Written Justifications:
   - Detail why AudioBook derives from ElectronicResource (digital streaming, platform fee, seat licensing).
   - Detail why Thesis derives from Resource (archival monograph, free of cost, no platform fee).
4. Key Architectural & Design Decisions:
   - Precision fixed-point currency safety (Q15).
   - Two-phase simulation for all-or-nothing batch processing (Q11).
   - Dynamic vendor price evaluation (Q12).
   - Title limit tracking via unique set collections (Q7).
   - Reversible order cancellations (Q8).
```

---

## Stage 9: Git History, Multi-Stage Commits & Packaging

### Prompt 9.1: Multi-Stage Staged Commit Strategy (4 Commits)
```text
Guide me through dividing my working repository into 4 logical, clean Git commits:
1. Commit 1 (Part A + G): Hierarchy classes (Journal, EBook, AudioBook, Thesis) and assert test setup.
   Message: "feat(hierarchy): implement Part A - add Journal, EBook, AudioBook, and Thesis classes"
2. Commit 2 (Part B + F): Hardcover pricing, bulk discounts, and multi-currency Money.
   Message: "feat(pricing): implement Part B (hardcover, bulk discounts) and Part F (multi-currency Money)"
3. Commit 3 (Part C + D): Quotas, title limits, cancellation, dept budgets, rollover, batch processing, and vendors.
   Message: "feat(acquisitions): implement Part C (quotas, cancellation, dept budgets, rollover) and Part D (batch, vendors)"
4. Commit 4 (Part E + Docs): Catalog searches, lending, save/load, demo.cpp, and ANSWERS.md.
   Message: "feat(catalog): implement Part E (searches, save/load, lending), assert test suite, demo, and ANSWERS.md"
Show me the exact git add commands for each stage.
```

### Prompt 9.2: Git Author Verification & Force Re-authoring
```text
I need to verify that all commits in my local repository show my real name and GitHub email before submission.
1. Show me how to check the current author details using `git log --format="%h - %an <%ae> - %s"`.
2. If the commits were authored with a placeholder or generic student email, provide a command to re-author them to:
   Name: <My Name>
   Email: <My GitHub Email>
3. Show me how to force-push the updated history to my GitHub repository.
```

### Prompt 9.3: Packaging for Submission with Hidden `.git` Folder
```text
Provide instructions and commands to package my assignment into a single zip file for submission:
1. Ensure the zip contains the complete BookManagement project directory.
2. Crucial: Ensure the hidden .git directory is fully included inside the zip archive without being stripped (explain why downloading ZIP from the GitHub web UI fails this requirement).
3. Provide the command on Windows (using tar or Compress-Archive) to create BookManagement_Assignment2.zip.
4. Provide a command to verify that .git/HEAD and objects exist inside the zip archive.
```

---

*This guide covers the entire software engineering and OOP lifecycle for Assignment 2.*
