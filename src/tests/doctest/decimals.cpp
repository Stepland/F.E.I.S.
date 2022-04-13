#include <doctest.h>

#include "../../special_numeric_types.hpp"

TEST_CASE("Decimals") {
    SUBCASE("can be constructed from") {
        SUBCASE("integers, exactly") {
            CHECK(Decimal{1} == Decimal{"1"});
            CHECK(Decimal{-2} == Decimal{"-2"});
        }
        SUBCASE("strings, exactly") {
            CHECK(Decimal{"0.0001"} == Decimal{1} / Decimal{10000});
        }
        SUBCASE("fractions, rounded to a given number of decimal places") {
            CHECK(convert_to_decimal({1,3}, 3) == Decimal{"0.333"});
            CHECK(convert_to_decimal({-1,3}, 3) == Decimal{"-0.333"});
            CHECK(convert_to_decimal({1,7}, 5) == Decimal{"0.14286"});
            CHECK(convert_to_decimal({123456,1000}, 1) == Decimal{"123.5"});
            CHECK(convert_to_decimal({1234,1000}, 1) == Decimal{"1.2"});
            CHECK(convert_to_decimal({1000,1234}, 1) == Decimal{"0.8"});
            CHECK(convert_to_decimal({1,777}, 1) == Decimal{"0.0"});
            CHECK(convert_to_decimal({1,777}, 5) == Decimal{"0.00129"});
            CHECK(convert_to_decimal({1,777}, 3) == Decimal{"0.001"});
        }
    }
}