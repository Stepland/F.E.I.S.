#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <ostream>
#include <type_traits>

#include <fmt/format.h>
#include <gmpxx.h>
#include <libmpdec++/decimal.hh>

using Decimal = decimal::Decimal;

class Fraction {
public:
    template<class ...Ts>
    Fraction(Ts&&... Args) requires (std::constructible_from<mpq_class, Ts...>) :
        value(std::forward<Ts>(Args)...)
    {
        value.canonicalize();
    };

    explicit Fraction(const unsigned long long a);
    Fraction(const unsigned long long a, const unsigned long long b);
    explicit Fraction(const long long a);
    Fraction(const long long a, const long long b);
    explicit Fraction(const Decimal& d);
    explicit operator std::int64_t() const;
    explicit operator std::uint64_t() const;
    explicit operator double() const;
    explicit operator float() const;

    const mpz_class& numerator() const;
    const mpz_class& denominator() const;

    Fraction& operator+=(const Fraction& rhs);
    Fraction& operator-=(const Fraction& rhs);
    Fraction& operator*=(const Fraction& rhs);
    Fraction& operator/=(const Fraction& rhs);
    bool operator==(const Fraction&) const;

    friend Fraction operator+(Fraction a, const Fraction& b);
    friend Fraction operator-(Fraction a, const Fraction& b);
    friend Fraction operator*(Fraction a, const Fraction& b);
    friend Fraction operator/(Fraction a, const Fraction& b);
    friend Fraction operator%(Fraction a, const Fraction& b);
    friend std::strong_ordering operator<=>(const Fraction& lhs, const Fraction& rhs);
    friend std::ostream& operator<<(std::ostream& os, const Fraction& f);
    friend struct fmt::formatter<Fraction>;

private:
    mpq_class value;
};

template <>
struct fmt::formatter<Fraction>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const Fraction& c, FormatContext& ctx) {
        return formatter<string_view>::format(c.value.get_str(), ctx);
    }
};


const auto mpz_uint64_max = mpz_class(fmt::format("{}", UINT64_MAX));
const auto mpz_int64_min = mpz_class(fmt::format("{}", INT64_MIN));
const auto mpz_int64_max = mpz_class(fmt::format("{}", INT64_MAX));

std::int64_t convert_to_i64(const mpz_class& z);
std::uint64_t convert_to_u64(const mpz_class& z);

Fraction floor_fraction(const Fraction& f);
Fraction round_fraction(const Fraction& f);
Fraction convert_to_fraction(const Decimal& d);

// Rounds a given beat to the nearest given division (defaults to nearest 1/240th)
Fraction round_beats(const Fraction& beats, const std::uint64_t denominator = 240);
Fraction floor_beats(const Fraction& beats, const std::uint64_t denominator = 240);

Decimal convert_to_decimal(const Fraction& f, unsigned int decimal_places);

template <>
struct fmt::formatter<Decimal>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const Decimal& d, FormatContext& ctx) {
        return formatter<string_view>::format(d.format("f"), ctx);
    }
};

/*
Stolen from :
https://github.com/progrock-libraries/kickstart/blob/master/source/library/kickstart/main_library/core/ns%E2%96%B8language/operations/intpow.hpp#L36

The original comment reads : 

    Essentially this is Horner's rule adapted to calculating a power, so that
    the number of floating point multiplications is at worst O(log₂n).

*/
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