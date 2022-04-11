#include <rapidcheck.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Numeric.h>

#include "../../better_notes.hpp"
#include "../../variant_visitor.hpp"

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

    template<>
    struct Arbitrary<better::Notes> {
        static Gen<better::Notes> arbitrary() {
            const auto raw_note = *
                gen::tuple(
                    gen::arbitrary<better::Position>(),
                    gen::positive<Fraction>(),
                    gen::arbitrary<Fraction>()
                )
            ;
            std::vector<std::tuple<better::Position, Fraction, Fraction>> raw_notes = {raw_note};
            std::array<Fraction, 16> last_note_end = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            better::Notes result;
            for (const auto& [position, delay, duration]: raw_notes) {
                if (duration > 0) {
                    const auto tail_6_notation = *gen::inRange<unsigned int>(0, 6);
                    const auto tail_tip = better::convert_6_notation_to_position(position, tail_6_notation);
                    auto& end = last_note_end[position.index()];
                    const auto time = end + delay;
                    end += delay + duration;
                    result.insert(better::LongNote{time, position, duration, tail_tip});
                } else {
                    auto& end = last_note_end[position.index()];
                    const auto time = end + delay;
                    end += delay;
                    result.insert(better::TapNote{time, position});
                }
            }
            return gen::just(result);
        }
    };
}