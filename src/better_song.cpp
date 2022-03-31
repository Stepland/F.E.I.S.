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

    nlohmann::ordered_json Song::dump_as_memon_1_0_0() const {
        nlohmann::ordered_json memon;
        memon["version"] = "1.0.0";
        auto json_metadata = dump_metadata_1_0_0();
        if (not json_metadata.empty()) {
            memon["metadata"] = json_metadata;
        }
        auto json_timing = nlohmann::ordered_json::object();
        if (timing) {
            json_timing.update(timing->dump_for_memon_1_0_0());
        }
        if (hakus) {
            json_timing["hakus"] = dump_hakus(*hakus);
        }
        if (not json_timing.empty()) {
            memon["timing"] = json_timing;
        }
        auto json_charts = nlohmann::ordered_json::object();
        for (const auto& [name, chart] : charts) {
            json_charts[name] = chart.dump_for_memon_1_0_0();
        }
        return memon;
    }
    
    nlohmann::ordered_json Song::dump_metadata_1_0_0() const {
        nlohmann::ordered_json json_metadata;
        if (not metadata.title.empty()) {
            json_metadata["title"] = metadata.title;
        }
        if (not metadata.artist.empty()) {
            json_metadata["artist"] = metadata.artist;
        }
        if (not metadata.audio.empty()) {
            json_metadata["audio"] = metadata.audio;
        }
        if (not metadata.jacket.empty()) {
            json_metadata["jacket"] = metadata.jacket;
        }
        if (metadata.use_preview_file) {
            if (not metadata.preview_file.empty()) {
                json_metadata["preview"] = metadata.preview_file;
            }
        } else {
            if (metadata.preview_loop.duration != 0) {
                json_metadata["preview"] = {
                    {"start", metadata.preview_loop.start.format("f")},
                    {"duration", metadata.preview_loop.duration.format("f")}
                };
            }
        }
        return json_metadata;
    };
}