#include "generators.hpp"
#include "rapidcheck/gen/Arbitrary.hpp"

namespace rc {
    template <>
    Gen<Fraction> gen::positive() {
        return gen::apply([](const Fraction& a, unsigned int b, unsigned int c) {
                return a + Fraction{std::min(b, c), std::max(b, c)};
            },
            gen::construct<Fraction>(gen::inRange<unsigned int>(0,100)),
            gen::inRange<unsigned int>(1,10),
            gen::inRange<unsigned int>(1,10)
        );
    };
}