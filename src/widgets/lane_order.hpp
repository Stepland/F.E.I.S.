#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <variant>

#include <toml++/toml.h>

namespace linear_view {
    namespace lane_order {
        struct Default {};
        struct Vertical {};
        struct Custom {
            Custom();
            explicit Custom(const std::string& as_string);
            std::array<std::optional<unsigned int>, 16> lane_to_button;
            std::map<unsigned int, unsigned int> button_to_lane;
            std::string as_string;
            void cleanup_string();
            void update_from_string();
            void update_from_array();
        };

        const std::array<std::string, 16> letters = {
            "1","2","3","4",
            "5","6","7","8",
            "9","a","b","c",
            "d","e","f","g"
        };

        const std::map<char, unsigned int> letter_to_index = {
            {'1', 0}, {'2', 1}, {'3', 2}, {'4', 3},
            {'5', 4}, {'6', 5}, {'7', 6}, {'8', 7},
            {'9', 8}, {'a', 9}, {'b', 10}, {'c', 11},
            {'d', 12}, {'e', 13}, {'f', 14}, {'g', 15},
        };
    }

    using LaneOrder = std::variant<lane_order::Default, lane_order::Vertical, lane_order::Custom>;

    namespace lane_order {
        LaneOrder load_from_v1_0_0_table(const toml::table& linear_view);
        void dump_as_v1_0_0(const LaneOrder& lane_order, toml::table& linear_view);
    }

    const LaneOrder default_lane_order = lane_order::Default{};
}