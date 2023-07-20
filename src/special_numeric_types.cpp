#include "special_numeric_types.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <gmpxx.h>
#include <stdexcept>

Fraction::Fraction(const unsigned long long a) {
    const unsigned long a_low = a & 0x00000000ffffffffULL;
    const unsigned long a_high = a >> 32;
    value = a_low + (mpq_class{a_high} << 32);
}

Fraction::Fraction(const unsigned long long a, const unsigned long long b) {
    value = Fraction{a}.value / Fraction{b}.value;
}

Fraction::Fraction(const long long a) {
    const unsigned long long abs = [&](){
        if (a == INT64_MIN) {
            return static_cast<unsigned long long>(INT64_MAX) + 1;
        } else {
            return static_cast<unsigned long long>(a < 0 ? -a : a);
        }
    }();
    value = Fraction{abs}.value;
    if (a < 0) {
        value = -value;
    }
}

Fraction::Fraction(const long long a, const long long b) {
    value = Fraction{a}.value / Fraction{b}.value;
}

Fraction::Fraction(const Decimal& d) {
    const auto reduced = d.reduce();
    const mpq_class sign = (reduced.sign() > 0 ? 1 : -1);
    const mpq_class coefficient = Fraction{reduced.coeff().u64()}.value;
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

Fraction::operator float() const {
    return static_cast<float>(value.get_d());
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
    const mpz_class dividend = na * db;
    const mpz_class divisor = nb * da;
    mpz_class remainder;
    mpz_mod(remainder.get_mpz_t(), dividend.get_mpz_t(), divisor.get_mpz_t());
    return Fraction{remainder, da * db};
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

std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    os << f.value.get_str();
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
    const mpz_class low = abs & 0x00000000ffffffffUL;
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
    const mpz_class low = z & 0x00000000ffffffffUL;
    const mpz_class high = z >> 32;
    const auto low_ul = low.get_ui();
    const auto high_ul = high.get_ui();
    const auto low_i64 = static_cast<std::uint64_t>(low_ul);
    const auto high_i64 = static_cast<std::uint64_t>(high_ul);
    return low_i64 + (high_i64 << 32);
}

Fraction floor_fraction(const Fraction& f) {
    mpz_class result;
    mpz_fdiv_q(result.get_mpz_t(), f.numerator().get_mpz_t(), f.denominator().get_mpz_t());
    return Fraction{result};
};

// Thanks python again !
// https://github.com/python/cpython/blob/f163ad22d3321cb9bb4e6cbaac5a723444641565/Lib/fractions.py#L612
Fraction round_fraction(const Fraction& f) {
    mpz_class floor, remainder;
    mpz_fdiv_qr(
        floor.get_mpz_t(),
        remainder.get_mpz_t(),
        f.numerator().get_mpz_t(),
        f.denominator().get_mpz_t()
    );
    if (remainder * 2 < f.denominator()) {
        return Fraction{floor};
    } else if (remainder * 2 > f.denominator()) {
        return Fraction{floor + 1};
    } else if (floor % 2 == 0) {  // Deal with the half case
        return Fraction{floor};
    } else {
        return floor + 1;
    }
};

Fraction ceil_fraction(const Fraction& f) {
    return floor_fraction(f) + 1;
}

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

Fraction round_beats(const Fraction& beats, const std::uint64_t denominator) {
    const auto actual_denominator = std::max<std::uint64_t>(denominator, 1);
    const auto nearest = round_fraction(beats * actual_denominator);
    return nearest / Fraction{actual_denominator};
};

Fraction floor_beats(const Fraction& beats, const std::uint64_t denominator) {
    const auto actual_denominator = std::max<std::uint64_t>(denominator, 1);
    const auto nearest = floor_fraction(beats * actual_denominator);
    return nearest / Fraction{actual_denominator};
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