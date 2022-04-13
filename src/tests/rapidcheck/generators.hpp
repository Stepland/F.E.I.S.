#pragma once

#include <cstddef>
#include <optional>

#include <rapidcheck.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Build.h>
#include <rapidcheck/gen/Container.h>
#include <rapidcheck/gen/Exec.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gen/Predicate.h>
#include <rapidcheck/gen/Transform.h>

#include "../../better_chart.hpp"
#include "../../better_hakus.hpp"
#include "../../better_note.hpp"
#include "../../better_notes.hpp"
#include "../../better_timing.hpp"
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
            return gen::apply([](const Fraction& a, unsigned int b, unsigned int c) {
                    return a + Fraction{std::min(b, c), std::max(b, c)};
                },
                gen::construct<Fraction>(gen::inRange<int>(-100,100)),
                gen::inRange<int>(0,10),
                gen::inRange<int>(1,10)
            );
        }
    };

    template <>
    Gen<Fraction> gen::nonNegative();

    template <>
    Gen<Fraction> gen::positive();

    template<>
    struct Arbitrary<better::TapNote> {
        static Gen<better::TapNote> arbitrary() {
            return gen::construct<better::TapNote>(
                gen::nonNegative<Fraction>(),
                gen::arbitrary<better::Position>()
            );
        }
    };

    template<>
    struct Arbitrary<better::LongNote> {
        static Gen<better::LongNote> arbitrary() {
            return gen::apply(
                [](
                    const Fraction& time,
                    const better::Position& position,
                    const Fraction& duration,
                    unsigned int tail_6_notation
                ){
                    const auto tail_tip = better::convert_6_notation_to_position(
                        position, tail_6_notation
                    );
                    return better::LongNote{
                        time,
                        position,
                        duration,
                        tail_tip,
                    };
                },
                gen::nonNegative<Fraction>(),
                gen::arbitrary<better::Position>(),
                gen::positive<Fraction>(),
                gen::inRange<unsigned int>(0, 6)
            );
        };
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
            return gen::exec([](){
                const auto raw_notes = *gen::container<std::vector<std::tuple<better::Position, Fraction, Fraction>>>(gen::tuple(
                    gen::arbitrary<better::Position>(),
                    gen::positive<Fraction>(),
                    gen::nonNegative<Fraction>()
                ));
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
                return result;
            });
        }; 
    };

    template<>
    struct Arbitrary<Decimal> {
        static Gen<Decimal> arbitrary() {
            return gen::map(
                gen::inRange<long>(-100000000L, 100000000L),
                [](long f){return Decimal(f) / 100000;}
            );
        }
    };

    template <>
    Gen<Decimal> gen::positive();

    template<>
    struct Arbitrary<better::BPMAtBeat> {
        static Gen<better::BPMAtBeat> arbitrary() {
            return gen::construct<better::BPMAtBeat>(
                gen::positive<Decimal>(),
                gen::nonNegative<Fraction>()
            );
        }
    };

    template<>
    struct Arbitrary<better::Timing> {
        static Gen<better::Timing> arbitrary() {
            return gen::construct<better::Timing>(
                gen::withSize([](std::size_t size) {
                    return gen::uniqueBy<std::vector<better::BPMAtBeat>>(
                        std::max(std::size_t(1), size),
                        gen::arbitrary<better::BPMAtBeat>(),
                        [](const better::BPMAtBeat& b){return b.get_beats();}
                    );
                }),
                gen::arbitrary<Decimal>()
            );
        }
    };

    template<>
    struct Arbitrary<Hakus> {
        static Gen<Hakus> arbitrary() {
            return gen::container<std::set<Fraction>>(gen::positive<Fraction>());
        }
    };

    template<class T>
    struct Arbitrary<std::optional<T>> {
        static Gen<std::optional<T>> arbitrary() {
            return gen::mapcat(
                gen::arbitrary<bool>(),
                 [](bool engaged) {
                    if (not engaged) {
                        return gen::construct<std::optional<T>>();
                    } else {
                        return gen::construct<std::optional<T>>(gen::arbitrary<T>());
                    }
                }
            );
        }
    };

    template<>
    struct Arbitrary<better::Chart> {
        static Gen<better::Chart> arbitrary() {
            return gen::construct<better::Chart>(
                gen::arbitrary<std::optional<Decimal>>(),
                gen::arbitrary<std::optional<better::Timing>>(),
                gen::arbitrary<std::optional<Hakus>>(),
                gen::arbitrary<better::Notes>()
            );
        }
    };
}