#pragma once

#include <compare>

#include <boost/multiprecision/gmp.hpp>
#include <libmpdec++/decimal.hh>

using Fraction = boost::multiprecision::mpq_rational;
using Decimal = decimal::Decimal;

inline std::strong_ordering operator<=>(const Fraction& lhs, const Fraction& rhs) {
    if (lhs < rhs) {
        return std::strong_ordering::less;
    } else if (lhs == rhs) {
        return std::strong_ordering::equal;
    } else {
        return std::strong_ordering::greater;
    }
};

Fraction operator%(Fraction a, const Fraction& b);
Fraction floor_fraction(Fraction f);
Fraction round_fraction(Fraction f);

// Rounds a given beat to the nearest given division (defaults to nearest 1/240th)
const auto round_beats = [](Fraction beats, unsigned int denominator = 240) {
    beats *= denominator;
    const auto nearest = round_fraction(beats);
    return nearest / Fraction{denominator};
};

const auto floor_beats = [](Fraction beats, unsigned int denominator = 240) {
    beats *= denominator;
    const auto nearest = floor_fraction(beats);
    return nearest / Fraction{denominator};
};