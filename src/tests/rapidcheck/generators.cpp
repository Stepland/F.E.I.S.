#include "generators.hpp"
#include "rapidcheck/gen/Arbitrary.hpp"
#include "rapidcheck/gen/Predicate.h"

namespace rc {
    template <>
    Gen<Fraction> gen::nonNegative() {
        return gen::apply([](const Fraction& a, unsigned int b, unsigned int c) {
                return a + Fraction{std::min(b, c), std::max(b, c)};
            },
            gen::construct<Fraction>(gen::inRange<int>(0,100)),
            gen::inRange<int>(1,10),
            gen::inRange<int>(0,10)
        );
    };

    template <>
    Gen<Fraction> gen::positive() {
        return gen::apply([](const Fraction& a, int b, int c) {
                return a + Fraction{std::min(b, c), std::max(b, c)};
            },
            gen::construct<Fraction>(gen::inRange<int>(0,100)),
            gen::inRange<int>(1,10),
            gen::inRange<int>(1,10)
        );
    };

    template <>
    Gen<Decimal> gen::positive() {
        return gen::map(
            gen::inRange<long>(1, 100000000L),
            [](long f){return Decimal(f) / 100000;}
        );
    };

    Gen<std::string> ascii_string() {
        return gen::container<std::string>(gen::inRange<std::string::value_type>(32, 127));
    };
}