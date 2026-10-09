#pragma once
// Money: fixed-point currency amount stored in minor units (e.g. paise/cents).
// Using an integer avoids floating-point rounding errors in cost/budget sums.

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    Money() : minor_(0), currency_("INR") {}

    // Construct from minor units: Money::fromMinor(12550) == 125.50
    static Money fromMinor(std::int64_t minor, std::string currency = "INR") {
        return Money(minor, std::move(currency));
    }
    // Construct from major + minor parts: Money::of(125, 50) == 125.50
    static Money of(std::int64_t major, int minor = 0, std::string currency = "INR");

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currency() const { return currency_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;  // "1234.50" / "-3.05"

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(const Money& o);
    Money& operator-=(const Money& o);
    Money& operator*=(std::int64_t k) { minor_ *= k; return *this; }

    friend Money operator+(Money a, const Money& b) { return a += b; }
    friend Money operator-(Money a, const Money& b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend bool operator==(const Money& a, const Money& b);
    friend bool operator!=(const Money& a, const Money& b);
    friend bool operator<(const Money& a, const Money& b);
    friend bool operator<=(const Money& a, const Money& b);
    friend bool operator>(const Money& a, const Money& b);
    friend bool operator>=(const Money& a, const Money& b);

private:
    explicit Money(std::int64_t minor, std::string currency = "INR")
        : minor_(minor), currency_(std::move(currency)) {}

    void checkSameCurrency(const Money& o) const;

    std::int64_t minor_ = 0;
    std::string currency_ = "INR";
};

std::ostream& operator<<(std::ostream& os, const Money& m);

}  // namespace bookmgmt
