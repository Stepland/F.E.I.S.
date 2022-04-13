#include "special_numeric_types.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>

Fraction::Fraction(const Decimal& d) {
    const auto reduced = d.reduce();
    const mpq_class sign = (reduced.sign() > 0 ? 1 : -1);
    const mpq_class coefficient = reduced.coeff().u64();
    const auto exponent = reduced.exponent();
    const mpq_class power = fast_pow(mpq_class(10), std::abs(exponent));
    if (exponent >= 0) {
        value = sign * coefficient * power;
    } else {
        value = sign * coefficient / power;
    }
}

Fraction::operator std::int64_t() const {
    return convert_to_i64(floor_fraction(*this).numerator());
};

Fraction::operator std::uint64_t() const {
    return convert_to_u64(floor_fraction(*this).numerator());
};

Fraction::operator double() const {
    return value.get_d();
};

const mpz_class& Fraction::numerator() const {
    return value.get_num();
}

const mpz_class& Fraction::denominator() const {
    return value.get_den();
}

Fraction& Fraction::operator+=(const Fraction& rhs) {
    value += rhs.value;
    return *this;
};

Fraction& Fraction::operator-=(const Fraction& rhs) {
    value -= rhs.value;
    return *this;
};

Fraction& Fraction::operator*=(const Fraction& rhs) {
    value *= rhs.value;
    return *this;
};

Fraction& Fraction::operator/=(const Fraction& rhs) {
    value /= rhs.value;
    return *this;
};

bool Fraction::operator==(const Fraction& rhs) const {
    return value == rhs.value;
};

Fraction operator+(Fraction lhs, const Fraction& rhs) {
    lhs+= rhs;
    return lhs;
};

Fraction operator-(Fraction lhs, const Fraction& rhs) {
    lhs -= rhs;
    return lhs;
};

Fraction operator*(Fraction lhs, const Fraction& rhs) {
    lhs *= rhs;
    return lhs;
};

Fraction operator/(Fraction lhs, const Fraction& rhs) {
    lhs /= rhs;
    return lhs;
};

// Thanks python !
// https://github.com/python/cpython/blob/f163ad22d3321cb9bb4e6cbaac5a723444641565/Lib/fractions.py#L533
//
// passing lhs by value helps optimize chained a+b+c, says cppreference
Fraction operator%(Fraction lhs, const Fraction& rhs) {
    const auto da = lhs.value.get_den();
    const auto db = rhs.value.get_den();
    const auto na = lhs.value.get_num();
    const auto nb = rhs.value.get_num();
    return Fraction{(na * db) % (nb * da), da * db};
};

std::strong_ordering operator<=>(const Fraction& lhs, const Fraction& rhs) {
    auto res = cmp(lhs.value, rhs.value);
    if (res < 0) {
        return std::strong_ordering::less;
    } else if (res == 0) {
        return std::strong_ordering::equal;
    } else {
        return std::strong_ordering::greater;
    }
};

std::ostream& operator<<(std::ostream& os, const Fraction& obj) {
    os << obj.value.get_str();
    return os;
}

std::int64_t convert_to_i64(const mpz_class& z) {
    if (z < mpz_int64_min or z > mpz_int64_max) {
        throw std::range_error(fmt::format(
            "number {} is too large to be represented by an std::int64_t",
            z.get_str()
        ));
    }
    /* In absolute value, INT64_MIN is greater than INT64_MAX, so I can't take
    the absolute value of INT64_MIN and convert it to std::int64_t without
    triggering a signed overflow (which is UB ?).
    So I deal with it separately */
    if (z == mpz_int64_min) {
        return INT64_MIN;
    }
    const bool positive = z >= 0;
    const mpz_class abs = positive ? z : -z;
    /* We can only get unsigned longs from GMP (32 garanteed bits), so we have
    to split between low and high */
    const mpz_class low = abs & INT64_C(0x00000000ffffffff);
    const mpz_class high = abs >> 32;
    const auto low_ul = low.get_ui();
    const auto high_ul = high.get_ui();
    const auto low_i64 = static_cast<std::int64_t>(low_ul);
    const auto high_i64 = static_cast<std::int64_t>(high_ul);
    const auto abs_i64 = low_i64 + (high_i64 << 32);
    if (positive) {
        return abs_i64;
    } else {
        return -abs_i64;
    }
}

std::uint64_t convert_to_u64(const mpz_class& z) {
    if (z < 0 or z > mpz_uint64_max) {
        throw std::range_error(fmt::format(
            "number {} is too large to be represented by an std::uint64_t",
            z.get_str()
        ));
    }
    /* We can only get unsigned longs from GMP (32 garanteed bits), so we have
    to split between low and high */
    const mpz_class low = z & INT64_C(0x00000000ffffffff);
    const mpz_class high = z >> 32;
    const auto low_ul = low.get_ui();
    const auto high_ul = high.get_ui();
    const auto low_i64 = static_cast<std::uint64_t>(low_ul);
    const auto high_i64 = static_cast<std::uint64_t>(high_ul);
    return low_i64 + (high_i64 << 32);
}

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

Decimal convert_to_decimal(const Fraction& f, unsigned int decimal_places) {
    const auto numerator = Decimal{f.numerator().get_str()};
    const auto denominator = Decimal{f.denominator().get_str()};
    const auto divided = numerator / denominator;
    if (decimal_places == 0) {
        return divided.to_integral();
    } else {
        const auto quantizer = Decimal{fmt::format("1.{:0{}}", 0, decimal_places)};
        return divided.quantize(quantizer);
    }
}