#include "better_beats.hpp"
#include <boost/multiprecision/gmp.hpp>
#include "json.hpp"

bool is_expressible_as_240th(const Fraction& beat) {
    return (
        (
            (240 * boost::multiprecision::numerator(beat))
            % boost::multiprecision::denominator(beat)
        ) == 0
    );
};

nlohmann::ordered_json beat_to_best_form(const Fraction& beat) {
    if (is_expressible_as_240th(beat)) {
        return nlohmann::ordered_json(
            (240 * boost::multiprecision::numerator(beat))
            / boost::multiprecision::denominator(beat)
        );
    } else {
        return beat_to_fraction_tuple(beat);
    };
};

nlohmann::ordered_json beat_to_fraction_tuple(const Fraction& beat) {
    const auto integer_part = static_cast<nlohmann::ordered_json::number_unsigned_t>(beat);
    const auto remainder = beat % 1;
    return {
        integer_part,
        static_cast<nlohmann::ordered_json::number_unsigned_t>(boost::multiprecision::numerator(remainder)),
        static_cast<nlohmann::ordered_json::number_unsigned_t>(boost::multiprecision::denominator(remainder)),
    };
};