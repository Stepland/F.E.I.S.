#include "special_numeric_types.hpp"

std::strong_ordering operator<=>(const Fraction& lhs, const Fraction& rhs) {
    if (lhs < rhs) {
        return std::strong_ordering::less;
    } else if (lhs == rhs) {
        return std::strong_ordering::equal;
    } else {
        return std::strong_ordering::greater;
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

Decimal convert_to_decimal(const Fraction& f, std::uint64_t precision) {
    const Fraction precision_mod = fast_pow(Fraction{1, 10}, precision);
    const Fraction floored = f - (f % precision_mod);
    return (
        Decimal{convert_to_int64(floored.get_num())}
        / Decimal{convert_to_int64(floored.get_den())}
    );
};

Fraction convert_to_fraction(const Decimal& d) {
    const auto reduced = d.reduce();
    const auto sign = reduced.sign();
    const auto exponent = reduced.exponent();
    const auto coefficient = reduced.coeff().u64();
    if (exponent >= 0) {
        return Fraction{sign > 0 ? 1 : -1} * Fraction{coefficient} * fast_pow(Fraction{10}, exponent);
    } else {
        return Fraction{sign > 0 ? 1 : -1} * Fraction{coefficient} / fast_pow(Fraction{10}, exponent);
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

mpz_class convert_to_mpz(std::uint64_t u) {
    auto low = static_cast<std::uint32_t>(u);
    auto high = static_cast<std::uint32_t>(u >> 32);
    return mpz_class{low} + (mpz_class{high} << 32);
};

mpz_class convert_to_mpz(std::int64_t i) {
    if (i < 0) {
        return -1 * convert_to_mpz(static_cast<std::uint64_t>(-1 * i));
    } else {
        return convert_to_mpz(static_cast<std::uint64_t>(i));
    }
};

std::uint64_t convert_to_uint64(const mpz_class& m) {
    if (m < 0) {
        throw std::invalid_argument("Cannot convert negative mpz to std::uint64_t");
    }

    const auto low = static_cast<std::uint64_t>(m.get_ui());
    const mpz_class high_m = m >> 32;
    const auto high = static_cast<std::uint64_t>(high_m.get_ui());
    return low + (high << 32);
};

std::int64_t convert_to_int64(const mpz_class& m) {
    if (m < 0) {
        return -1 * static_cast<std::int64_t>(convert_to_uint64(-1 * m));
    } else {
        return static_cast<std::int64_t>(convert_to_uint64(m));
    }
};