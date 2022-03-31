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

        nlohmann::ordered_json dump_for_memon_1_0_0() const;
    };
}