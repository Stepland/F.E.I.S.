#include <doctest.h>

#include "../../special_numeric_types.hpp"

TEST_CASE("Fractions") {
    SUBCASE("can be exactly constructed from") {
        SUBCASE("integers") {
            CHECK(Fraction{1} == Fraction{1,1});
            CHECK(Fraction{-2} == Fraction{-2,1});
        }
        SUBCASE("std::uint64_t") {
            CHECK(Fraction{UINT64_MAX, UINT64_C(1)} == Fraction{"18446744073709551615/1"});
        }
        SUBCASE("long long") {
            CHECK(Fraction{-8446744073709551615LL} == Fraction{"-8446744073709551615/1"});
            CHECK(Fraction{-1234567891234567891LL} == Fraction{"-1234567891234567891/1"});
            CHECK(Fraction{1234567891234567LL, 1234567LL} == Fraction{"1234567891234567/1234567"});
        }
        SUBCASE("unsigned long long") {
            CHECK(Fraction{18446744073709551615ULL} == Fraction{"18446744073709551615/1"});
            CHECK(Fraction{12345678912345678912ULL} == Fraction{"12345678912345678912/1"});
            CHECK(Fraction{1234567891234567ULL, 1234567ULL} == Fraction{"1234567891234567/1234567"});
        }
        SUBCASE("decimals") {
            CHECK(Fraction{Decimal{"0.0000000000000000001"}} == Fraction{"1/10000000000000000000"});
            CHECK(Fraction{Decimal{"12345.6789"}} == Fraction{123456789,10000});
        }
    }
    SUBCASE("can be cast to") {
        SUBCASE("std::int64_t, returing the integral part") {
            CHECK(static_cast<std::int64_t>(Fraction{1,2}) == INT64_C(0));
            CHECK(static_cast<std::int64_t>(Fraction{2,3}) == INT64_C(0));
            CHECK(static_cast<std::int64_t>(Fraction{-5,2}) == INT64_C(-3));
            CHECK(static_cast<std::int64_t>(Fraction{"-9223372036854775808/1"}) == INT64_MIN);
            CHECK(static_cast<std::int64_t>(Fraction{"9223372036854775807/1"}) == INT64_MAX);
            CHECK(static_cast<std::int64_t>(Fraction{"-9000000000000000000/1"}) == INT64_C(-9000000000000000000));
            CHECK(static_cast<std::int64_t>(Fraction{"9000000000000000000/1"}) == INT64_C(9000000000000000000));
        }
        SUBCASE("std::uint64_t, returing the (positive) integral part") {
            CHECK(static_cast<std::uint64_t>(Fraction{1,2}) == UINT64_C(0));
            CHECK(static_cast<std::uint64_t>(Fraction{5,2}) == UINT64_C(2));
            CHECK(static_cast<std::uint64_t>(Fraction{"18446744073709551615/1"}) == UINT64_MAX);
            CHECK(static_cast<std::uint64_t>(Fraction{"12345678912345678912/1"}) == UINT64_C(12345678912345678912));
        }
        SUBCASE("doubles, returing the approximated value") {
            CHECK(static_cast<double>(Fraction{1,10}) == doctest::Approx(0.1));
            CHECK(static_cast<double>(Fraction{31,10}) == doctest::Approx(3.1));
            CHECK(static_cast<double>(Fraction{-1,10}) == doctest::Approx(-0.1));
        }
    }
    SUBCASE("are canonicalized upon creation") {
        CHECK(Fraction(0,4) == Fraction(0,1));
        CHECK(Fraction{2,4} == Fraction{1,2});
        CHECK(Fraction{1,-2} == Fraction{-1,2});
    }
    SUBCASE("are correctly compared") {
        CHECK(Fraction{1,2} > Fraction{1,3});
        CHECK(Fraction{-1,2} < Fraction{1,3});
        CHECK(Fraction{12,2} < Fraction{28, 4});
    }
    SUBCASE("are not turned into another type after binary operands") {
        CHECK(std::is_same_v<decltype(Fraction{1,2} + Fraction{1,3}), Fraction>);
        CHECK(std::is_same_v<decltype(Fraction{1,2} - Fraction{1,3}), Fraction>);
        CHECK(std::is_same_v<decltype(Fraction{1,2} * Fraction{1,3}), Fraction>);
        CHECK(std::is_same_v<decltype(Fraction{1,2} / Fraction{1,3}), Fraction>);
        CHECK(std::is_same_v<decltype(Fraction{1,2} % Fraction{1,3}), Fraction>);
    }
    SUBCASE("add correctly") {
        CHECK(Fraction{1,2} + Fraction{1,3} == Fraction{5,6});
        CHECK(Fraction{-1,2} + Fraction{1,2} == Fraction{0,1});
    }
    SUBCASE("subtract correctly") {
        CHECK(Fraction{1,2} - Fraction{1,3} == Fraction{1,6});
        CHECK(Fraction{-1,2} - Fraction{1,2} == Fraction{-1,1});
    }
    SUBCASE("multiply correctly") {
        CHECK(Fraction{1,2} * Fraction{1,7} == Fraction{1,14});
        CHECK(Fraction{-1,2} * Fraction{1,2} == Fraction{-1,4});
    }
    SUBCASE("divide correctly") {
        CHECK(Fraction{1,2} / Fraction{1,7} == Fraction{7,2});
        CHECK(Fraction{-1,2} / Fraction{1,2} == Fraction{-1,1});
    }
    SUBCASE("are modulo'ed correctly") {
        CHECK(Fraction{3,2} % Fraction{1} == Fraction{1,2});
        CHECK(Fraction{23,14} % Fraction{2,5} == Fraction{3,70});
        CHECK(Fraction{-7,4} % Fraction{1} == Fraction{1,4});
    }
    SUBCASE("are floor'ed correctly") {
        CHECK(floor_fraction(Fraction{1,2}) == Fraction{0});
        CHECK(floor_fraction(Fraction{23,14}) == Fraction{1});
        CHECK(floor_fraction(Fraction{-7,4}) == Fraction{-2});
    }
    SUBCASE("are rounded correctly") {
        CHECK(round_fraction(Fraction{10,20}) == Fraction{0});
        CHECK(round_fraction(Fraction{11,20}) == Fraction{1});
        CHECK(round_fraction(Fraction{-10,20}) == Fraction{0});
        CHECK(round_fraction(Fraction{-11,20}) == Fraction{-1});
    }

    SUBCASE("support binary operand with") {
        SUBCASE("integer literals") {
            CHECK(Fraction{1,2} + 1 == Fraction{3,2});
            CHECK(Fraction{1,3} - 1 == Fraction{-2,3});
            CHECK(Fraction{1,4} * 2 == Fraction{1,2});
            CHECK(Fraction{1,5} / 2 == Fraction{1,10});
        }
        SUBCASE("floating point literals") {
            CHECK(Fraction{1,2} + 1.0 == Fraction{3,2});
        }
    }
}