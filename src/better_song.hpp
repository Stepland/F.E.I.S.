#pragma once

#include <filesystem>
#include <map>
#include <memory>
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

    std::tuple<int, std::string> difficulty_name_comp_key(const std::string& s);

    struct OrderByDifficultyName {
        bool operator()(const std::string& a, const std::string& b) const;
    };

    struct Song {
        std::map<std::string, better::Chart, OrderByDifficultyName> charts;
        Metadata metadata;
        std::shared_ptr<Timing> timing = std::make_shared<Timing>();
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
        - "data" is an *array* of charts
        - the difficulty name of a chart is stored as "dif_name" in the chart
          object
        - the album cover path field is named "jacket path"
        */
        static Song load_from_memon_legacy(const nlohmann::json& memon);

        bool operator==(const Song&) const;
        friend std::ostream& operator<<(std::ostream& out, const Song& s);
    };

    Note load_legacy_note(const nlohmann::json& legacy_note, std::uint64_t resolution);

    Position convert_legacy_memon_tail_index_to_position(const Position& pos, std::uint64_t tail_index);
}

template <>
struct fmt::formatter<better::Song>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::Song& s, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "Song(charts: {}, metadata: {}, timing: {}, hakus: {})",
            s.charts,
            s.metadata,
            *s.timing,
            s.hakus
        );
    }
};