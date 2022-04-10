#include <rapidcheck.h>

#include "../../better_note.hpp"
#include "../../special_numeric_types.hpp"
#include "rapidcheck/gen/Arbitrary.h"
#include "rapidcheck/gen/Numeric.h"

namespace rc {
    template<>
    struct Arbitrary<better::Position> {
        static Gen<better::Position> arbitrary() {
            return gen::construct<better::Position>(
                gen::inRange<unsigned int>(0, 16)
            );
        }
    };

    template<>
    struct Arbitrary<Fraction> {
        static Gen<Fraction> arbitrary() {
            return gen::apply([](const Fraction& a, const Fraction& b) {
                    return a + b;
                },
                gen::cast<Fraction>(
                    gen::nonNegative<unsigned>()
                ),
                gen::construct<Fraction>(
                    gen::nonNegative<unsigned>(),
                    gen::positive<unsigned>()
                )
            );
        }

        static Gen<Fraction> positive() {
            return gen::apply([](const Fraction& a, const Fraction& b) {
                    return a + b;
                },
                gen::cast<Fraction>(
                    gen::nonNegative<unsigned>()
                ),
                gen::construct<Fraction>(
                    gen::positive<unsigned>(),
                    gen::positive<unsigned>()
                )
            );
        }
    };

    template<>
    struct Arbitrary<better::TapNote> {
        static Gen<better::TapNote> arbitrary() {
            return gen::construct<better::TapNote>(
                gen::arbitrary<Fraction>(),
                gen::arbitrary<better::Position>()
            );
        }
    };

    template<>
    struct Arbitrary<better::LongNote> {
        static Gen<better::LongNote> arbitrary() {
            const auto pos = *gen::arbitrary<better::Position>();
            const auto tail_6_notation = *gen::inRange<unsigned int>(0, 6);
            const auto tail_pos = better::convert_6_notation_to_position(pos, tail_6_notation);
            return gen::construct<better::LongNote>(
                gen::arbitrary<Fraction>(),
                gen::just(pos),
                gen::positive<Fraction>(),
                gen::just(tail_pos)
            );
        }
    };

    template<>
    struct Arbitrary<better::Note> {
        static Gen<better::Note> arbitrary() {
            return gen::oneOf(
                gen::construct<better::Note>(gen::arbitrary<better::TapNote>()),
                gen::construct<better::Note>(gen::arbitrary<better::LongNote>())
            );
        }
    };
}