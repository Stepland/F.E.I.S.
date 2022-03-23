#include "special_numeric_types.hpp"
#include <boost/multiprecision/gmp.hpp>

// Thanks python !
// https://github.com/python/cpython/blob/f163ad22d3321cb9bb4e6cbaac5a723444641565/Lib/fractions.py#L533
//
// passing lhs by value helps optimize chained a+b+c, says cppreference
Fraction operator%(Fraction lhs, const Fraction& rhs) {
    const auto da = boost::multiprecision::denominator(lhs);
    const auto db = boost::multiprecision::denominator(rhs);
    const auto na = boost::multiprecision::numerator(lhs);
    const auto nb = boost::multiprecision::numerator(rhs);
    return Fraction{(na * db) % (nb * da), da * db};
}

Fraction floor_fraction(Fraction f) {
    return f - (f % Fraction{1});
};

Fraction round_fraction(Fraction f) {
    return floor_fraction(f + Fraction{1, 2});
}