#include "better_chart.hpp"

#include <cstdint>
#include <json.hpp>
#include <memory>
#include <optional>

#include "json_decimal_handling.hpp"

namespace better {
    nlohmann::ordered_json Chart::dump_to_memon_1_0_0(
        const nlohmann::ordered_json& fallback_timing_object
    ) const {
        nlohmann::ordered_json json_chart;
        if (level) {
            json_chart["level"] = level->format("f");
        }
        auto chart_timing = dump_memon_1_0_0_timing_object(timing, hakus, fallback_timing_object);
        if (not chart_timing.empty()) {
            json_chart["timing"] = chart_timing;
        }
        json_chart["notes"] = notes->dump_to_memon_1_0_0();

        return json_chart;
    };

    Chart Chart::load_from_memon_1_0_0(const nlohmann::json& json, const nlohmann::json& fallback_timing) {
        std::optional<Decimal> level;
        if (json.contains("level")) {
            level = load_as_decimal(json["level"]);
        }
        std::uint64_t chart_resolution = 240;
        if (json.contains("resolution")) {
            chart_resolution = json["resolution"].get<std::uint64_t>();
        }
        std::optional<std::shared_ptr<Timing>> timing;
        std::optional<Hakus> hakus;
        if (json.contains("timing")) {
            auto chart_timing = fallback_timing;
            chart_timing.update(json["timing"]);
            timing = std::make_shared<Timing>(Timing::load_from_memon_1_0_0(chart_timing));
            /*
            Don't re-use the merged 'chart_timing' here :
            
            'fallback_timing' might contain hakus from the song-wise timing
            object and we don't want to take these into account when looking
            for chart-specific ones
            */
            hakus = load_hakus(json["timing"]);
        }
        return Chart{
            .level = level,
            .timing = timing,
            .hakus = hakus,
            .notes = std::make_shared<Notes>(Notes::load_from_memon_1_0_0(json.at("notes"), chart_resolution))
        };
    }

    Chart Chart::load_from_memon_legacy(const nlohmann::json& json) {
        return Chart{
            .level = Decimal{json["level"].get<int>()},
            .timing = {},
            .hakus = {},
            .notes = std::make_shared<Notes>(Notes::load_from_memon_legacy(
                json.at("notes"),
                json.at("resolution").get<std::uint64_t>()
            ))
        };
    }

    std::ostream& operator<<(std::ostream& out, const better::Chart& c) {
        out << fmt::format("{}", c);
        return out;
    };

    nlohmann::ordered_json remove_keys_already_in_fallback(
        const nlohmann::ordered_json& object,
        const nlohmann::ordered_json& fallback
    ) {
        nlohmann::ordered_json j;
        for (const auto& [key, value] : object.items()) {
            if (not fallback.contains(key)) {
                j[key] = value;
            } else {
                if (fallback[key] != value) {
                    j[key] = value;
                }
            }
        }
        return j;
    };

    nlohmann::ordered_json dump_memon_1_0_0_timing_object(
        const std::optional<std::shared_ptr<better::Timing>>& timing,
        const std::optional<Hakus>& hakus,
        const nlohmann::ordered_json& fallback_timing_object
    ) {
        auto complete_song_timing = nlohmann::ordered_json::object();
        if (timing.has_value()) {
            complete_song_timing.update((**timing).dump_to_memon_1_0_0());
        }
        if (hakus.has_value()) {
            complete_song_timing["hakus"] = dump_hakus(*hakus);
        }
        return remove_keys_already_in_fallback(complete_song_timing, fallback_timing_object);
    };
}