#include "better_song.hpp"

#include <stdexcept>

#include <fmt/core.h>
#include <SFML/System/Time.hpp>

#include "better_hakus.hpp"
#include "better_timing.hpp"
#include "json.hpp"
#include "src/better_chart.hpp"
#include "src/better_hakus.hpp"
#include "src/better_metadata.hpp"
#include "src/better_note.hpp"
#include "src/special_numeric_types.hpp"
#include "std_optional_extras.hpp"
#include "variant_visitor.hpp"

namespace better {
    std::string stringify_level(std::optional<Decimal> level) {
        return stringify_or(level, "(no level defined)");
    };

    nlohmann::ordered_json Song::dump_to_memon_1_0_0() const {
        nlohmann::ordered_json memon;
        memon["version"] = "1.0.0";
        auto json_metadata = metadata.dump_to_memon_1_0_0();
        if (not json_metadata.empty()) {
            memon["metadata"] = json_metadata;
        }
        nlohmann::json fallback_timing_object = R"(
            {"offset": "0", "bpms": [{"beat": 0, "bpm": "120"}]}
        )"_json;
        auto song_timing = dump_memon_1_0_0_timing_object(timing, hakus, fallback_timing_object);
        if (not song_timing.empty()) {
            memon["timing"] = song_timing;
        }
        fallback_timing_object.update(song_timing);
        auto json_charts = nlohmann::ordered_json::object();
        for (const auto& [name, chart] : charts) {
            json_charts[name] = chart.dump_to_memon_1_0_0(fallback_timing_object);
        }
        return memon;
    }

    Song Song::load_from_memon(const nlohmann::json& memon) {
        if (not memon.is_object()) {
            throw std::invalid_argument(
                "The json file you tried to load does not contain an object "
                "at the top level. This is required for a json file to be a "
                "valid memon file."
            );
        }
        if (not memon.contains("version")) {
            return Song::load_from_memon_legacy(memon);
        }
        if (not memon["version"].is_string()) {
            throw std::invalid_argument(
                "The json file you tried to load has a 'version' key at the "
                "top level but its associated value is not a string. This is "
                "required for a json file to be a valid memon file."
            );
        }
        const auto version = memon["version"].get<std::string>();
        if (version == "1.0.0") {
            return Song::load_from_memon_1_0_0(memon);
        } else if (version == "0.3.0") {
            return Song::load_from_memon_0_3_0(memon);
        } else if (version == "0.2.0") {
            return Song::load_from_memon_0_2_0(memon);
        } else if (version == "0.1.0") {
            return Song::load_from_memon_0_1_0(memon);
        } else {
            throw std::invalid_argument(fmt::format(
                "Unknown memon version : {}",
                version
            ));
        }
    };

    Song Song::load_from_memon_1_0_0(const nlohmann::json& memon) {

    };

    Song Song::load_from_memon_0_3_0(const nlohmann::json& memon) {

    };

    Song Song::load_from_memon_0_2_0(const nlohmann::json& memon) {

    };

    Song Song::load_from_memon_0_1_0(const nlohmann::json& memon) {

    };

    Song Song::load_from_memon_legacy(const nlohmann::json& memon) {
        const auto json_metadata = memon["metadata"];
        const auto metadata = Metadata::load_from_memon_legacy(memon["metadata"]);
        const auto bpm = Decimal{json_metadata["BPM"].get<std::string>()};
        const auto offset = convert_to_fraction(Decimal{json_metadata["offset"].get<std::string>()});
        const auto timing = Timing::load_from_memon_legacy(bpm, offset);
        Song song{
            .charts = {},
            .metadata = metadata,
            .timing = timing,
            .hakus = {}
        };
        for (const auto& chart_json : memon["data"]) {
            const auto dif = chart_json["dif_name"].get<std::string>();
            const auto chart = Chart::load_from_memon_legacy(chart_json, timing);
            song.charts[dif] = chart;
        }
        return song;
    };
}