#include "special_numeric_types.hpp"

std::strong_ordering operator<=>(const Fraction& lhs, const Fraction& rhs) {
    const auto res = cmp(lhs, rhs);
    if (res > 0) {
        return std::strong_ordering::greater;
    } else if (res == 0) {
        return std::strong_ordering::equal;
    } else {
        return std::strong_ordering::less;
    }
};

// Thanks python !
// https://github.com/python/cpython/blob/f163ad22d3321cb9bb4e6cbaac5a723444641565/Lib/fractions.py#L533
//
// passing lhs by value helps optimize chained a+b+c, says cppreference
Fraction operator%(Fraction lhs, const Fraction& rhs) {
    const auto da = lhs.get_den();
    const auto db = rhs.get_den();
    const auto na = lhs.get_num();
    const auto nb = rhs.get_num();
    return Fraction{(na * db) % (nb * da), da * db};
};

Fraction floor_fraction(const Fraction& f) {
    return f - (f % Fraction{1});
};

Fraction round_fraction(const Fraction& f) {
    return floor_fraction(f + Fraction{1, 2});
};

Fraction convert_to_fraction(const Decimal& d) {
    const auto reduced = d.reduce();
    const auto sign = reduced.sign();
    const auto exponent = reduced.exponent();
    const auto coefficient = reduced.coeff().u64();
    if (exponent >= 0) {
        return Fraction{sign > 0 ? 1 : -1} * Fraction{coefficient} * fast_pow(Fraction{10}, exponent);
    } else {
        return Fraction{sign > 0 ? 1 : -1} * Fraction{coefficient} / fast_pow(Fraction{10}, -exponent);
    }
};

Fraction round_beats(Fraction beats, std::uint64_t denominator) {
    beats *= denominator;
    const auto nearest = round_fraction(beats);
    return nearest / Fraction{denominator};
};

Fraction floor_beats(Fraction beats, std::uint64_t denominator) {
    beats *= denominator;
    const auto nearest = floor_fraction(beats);
    return nearest / Fraction{denominator};
};