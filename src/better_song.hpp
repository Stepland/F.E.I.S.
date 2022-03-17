#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>

#include <SFML/System/Time.hpp>

#include "better_notes.hpp"
#include "better_timing.hpp"
#include "special_numeric_types.hpp"

namespace better {
    struct Chart {
        std::optional<Decimal> level;
        Timing timing;
        std::optional<std::set<Fraction>> hakus;
        Notes notes;

        std::optional<sf::Time> time_of_last_event() const;
    };

    class PreviewLoop {
    public:
        PreviewLoop(Decimal start, Decimal duration);
    private:
        Decimal start;
        Decimal duration;
    };

    struct Metadata {
        std::optional<std::string> title;
        std::optional<std::string> artist;
        std::optional<std::filesystem::path> audio;
        std::optional<std::filesystem::path> jacket;
        std::optional<std::variant<PreviewLoop, std::filesystem::path>> preview;
    };

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
    };
}