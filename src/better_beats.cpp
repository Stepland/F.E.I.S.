#include "better_beats.hpp"

#include <boost/multiprecision/gmp.hpp>
#include <fmt/core.h>
#include <json.hpp>
#include <memory>
#include <stdexcept>

#include "special_numeric_types.hpp"

bool is_expressible_as_240th(const Fraction& beat) {
    return (240 * beat.numerator()) % beat.denominator() == 0;
};

nlohmann::ordered_json beat_to_best_form(const Fraction& beat) {
    if (is_expressible_as_240th(beat)) {
        return nlohmann::ordered_json(
            (240 * convert_to_u64(beat.numerator())) / convert_to_u64(beat.denominator())
        );
    } else {
        return beat_to_fraction_tuple(beat);
    };
};

nlohmann::ordered_json beat_to_fraction_tuple(const Fraction& beat) {
    const auto integral_part = static_cast<std::uint64_t>(beat);
    const auto remainder = beat % 1;
    return {
        integral_part,
        convert_to_u64(remainder.numerator()),
        convert_to_u64(remainder.denominator()),
    };
};

Fraction load_memon_1_0_0_beat(const nlohmann::json& json, std::uint64_t resolution) {
    if (json.is_number()) {
        return Fraction{
            json.get<std::uint64_t>(),
            resolution
        };
    } else if (json.is_array()) {
        return (
            json[0].get<std::uint64_t>()
            + Fraction{
                json[1].get<std::uint64_t>(),
                json[2].get<std::uint64_t>()
            }
        );
    } else {
        throw std::invalid_argument(fmt::format(
            "Found an unexpected json value when trying to read a symbolic "
            "time, expected an 3-int array or a number but found {}",
            json
        ));
    }
}