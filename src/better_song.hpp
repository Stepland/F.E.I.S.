#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>

#include <json.hpp>
#include <SFML/System/Time.hpp>

#include "better_chart.hpp"
#include "better_hakus.hpp"
#include "better_metadata.hpp"
#include "better_notes.hpp"
#include "better_timing.hpp"
#include "special_numeric_types.hpp"

namespace better {
    std::string stringify_level(std::optional<Decimal> level);

    const auto difficulty_name_comp_key = [](const std::string& s) {
        if (s == "BSC") {
            return std::make_tuple(1, std::string{});
        } else if (s == "ADV") {
            return std::make_tuple(2, std::string{});
        } else if (s == "EXT") {
            return std::make_tuple(3, std::string{});
        } else {
            return std::make_tuple(4, s);
        }
    };

    const auto order_by_difficulty_name = [](const std::string& a, const std::string& b) {
        return difficulty_name_comp_key(a) < difficulty_name_comp_key(b);
    };

    struct Song {
        std::map<
            std::string,
            better::Chart,
            decltype(order_by_difficulty_name)
        > charts{order_by_difficulty_name};
        Metadata metadata;
        Timing timing;
        std::optional<Hakus> hakus;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        /*
        Read the json file as memon by trying to guess version.
        Throws various exceptions on error
        */
        static Song load_from_memon(const nlohmann::json& memon);

        /*
        Read the json file as memon v1.0.0.
        https://memon-spec.readthedocs.io/en/latest/changelog.html#v1-0-0
        */
        static Song load_from_memon_1_0_0(const nlohmann::json& memon);

        /*
        Read the json file as memon v0.3.0.
        https://memon-spec.readthedocs.io/en/latest/changelog.html#v0-3-0
        */
        static Song load_from_memon_0_3_0(const nlohmann::json& memon);

        /*
        Read the json file as memon v0.2.0.
        https://memon-spec.readthedocs.io/en/latest/changelog.html#v0-2-0
        */
        static Song load_from_memon_0_2_0(const nlohmann::json& memon);

        /*
        Read the json file as memon v0.1.0.
        https://memon-spec.readthedocs.io/en/latest/changelog.html#v0-1-0
        */
        static Song load_from_memon_0_1_0(const nlohmann::json& memon);

        /*
        Read the json file as a "legacy" (pre-versionning) memon file.

        Notable quirks of this archaïc schema :
        - "data" is an array of charts
        - the difficulty name of a chart is stored as "dif_name" in the chart
            object
        - the album cover path field is named "jacket path"
        */
        static Song load_from_memon_legacy(const nlohmann::json& memon);
    };

    Note load_legacy_note(const nlohmann::json& legacy_note, unsigned int resolution);

    Position legacy_memon_tail_index_to_position(const Position& pos, unsigned int tail_index);
}