#include "better_song.hpp"

#include <SFML/System/Time.hpp>

#include "better_hakus.hpp"
#include "json.hpp"
#include "src/better_hakus.hpp"
#include "std_optional_extras.hpp"
#include "variant_visitor.hpp"

namespace better {
    std::string stringify_level(std::optional<Decimal> level) {
        return stringify_or(level, "(no level defined)");
    };

    nlohmann::ordered_json Song::dump_for_memon_1_0_0() const {
        nlohmann::ordered_json memon;
        memon["version"] = "1.0.0";
        auto json_metadata = metadata.dump_for_memon_1_0_0();
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
            json_charts[name] = chart.dump_for_memon_1_0_0(fallback_timing_object);
        }
        return memon;
    }
}