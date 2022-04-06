#pragma once

#include <compare>
#include <cstdint>

#include <gmpxx.h>
#include <libmpdec++/decimal.hh>

using Fraction = mpq_class;
using Decimal = decimal::Decimal;

std::strong_ordering operator<=>(const Fraction& lhs, const Fraction& rhs);
Fraction operator%(Fraction a, const Fraction& b);
Fraction floor_fraction(const Fraction& f);
Fraction round_fraction(const Fraction& f);
Decimal convert_to_decimal(const Fraction& f, std::uint64_t precision);
Fraction convert_to_fraction(const Decimal& d);

// Rounds a given beat to the nearest given division (defaults to nearest 1/240th)
Fraction round_beats(Fraction beats, std::uint64_t denominator = 240);
Fraction floor_beats(Fraction beats, std::uint64_t denominator = 240);

// Stolen from :
// https://github.com/progrock-libraries/kickstart/blob/master/source/library/kickstart/main_library/core/ns%E2%96%B8language/operations/intpow.hpp#L36
// Essentially this is Horner's rule adapted to calculating a power, so that the
// number of floating point multiplications is at worst O(log₂n).
template<class Number>
Number fast_pow(const Number base, const std::uint64_t exponent) {
    Number result = 1;
    Number weight = base;
    for (std::uint64_t n = exponent; n != 0; weight *= weight) {
        if(n % 2 != 0) {
           result *= weight;
        }
        n /= 2;
    }
    return result;
};

mpz_class convert_to_mpz(std::uint64_t u);
mpz_class convert_to_mpz(std::int64_t i);
std::uint64_t convert_to_uint64(const mpz_class& m);
std::int64_t convert_to_int64(const mpz_class& m);