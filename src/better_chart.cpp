#include "better_chart.hpp"

#include <json.hpp>

namespace better {
    nlohmann::ordered_json Chart::dump_for_memon_1_0_0(
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
        json_chart["notes"] = notes.dump_for_memon_1_0_0();
    };

    nlohmann::ordered_json remove_common_keys(
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
        const better::Timing& timing,
        const std::optional<Hakus>& hakus,
        const nlohmann::ordered_json& fallback_timing_object
    ) {
        auto complete_song_timing = nlohmann::ordered_json::object();
        complete_song_timing.update(timing.dump_for_memon_1_0_0());
        if (hakus) {
            complete_song_timing["hakus"] = dump_hakus(*hakus);
        }
        return remove_common_keys(complete_song_timing, fallback_timing_object);
    };
}