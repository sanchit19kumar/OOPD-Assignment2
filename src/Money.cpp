#include "bookmgmt/Money.h"

#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

void Money::checkSameCurrency(const Money& o) const {
    if (currency_ != o.currency_) {
        throw CurrencyMismatchError(currency_, o.currency_);
    }
}

Money Money::of(std::int64_t major, int minor, std::string currency) {
    if (minor < 0 || minor > 99)
        throw std::invalid_argument("minor part must be in 0..99");
    const std::int64_t sign = major < 0 ? -1 : 1;
    return Money(major * 100 + sign * minor, std::move(currency));
}

std::string Money::toString() const {
    const std::int64_t absMinor = minor_ < 0 ? -minor_ : minor_;
    std::string s = std::to_string(absMinor / 100);
    const auto frac = absMinor % 100;
    s += '.';
    if (frac < 10) s += '0';
    s += std::to_string(frac);
    return minor_ < 0 ? "-" + s : s;
}

Money& Money::operator+=(const Money& o) {
    checkSameCurrency(o);
    minor_ += o.minor_;
    return *this;
}

Money& Money::operator-=(const Money& o) {
    checkSameCurrency(o);
    minor_ -= o.minor_;
    return *this;
}

bool operator==(const Money& a, const Money& b) {
    a.checkSameCurrency(b);
    return a.minor_ == b.minor_;
}

bool operator!=(const Money& a, const Money& b) {
    a.checkSameCurrency(b);
    return a.minor_ != b.minor_;
}

bool operator<(const Money& a, const Money& b) {
    a.checkSameCurrency(b);
    return a.minor_ < b.minor_;
}

bool operator<=(const Money& a, const Money& b) {
    a.checkSameCurrency(b);
    return a.minor_ <= b.minor_;
}

bool operator>(const Money& a, const Money& b) {
    a.checkSameCurrency(b);
    return a.minor_ > b.minor_;
}

bool operator>=(const Money& a, const Money& b) {
    a.checkSameCurrency(b);
    return a.minor_ >= b.minor_;
}

std::ostream& operator<<(std::ostream& os, const Money& m) {
    return os << m.toString();
}

}  // namespace bookmgmt
