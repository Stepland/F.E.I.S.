#include "better_metadata.hpp"

namespace better {
    nlohmann::ordered_json Metadata::dump_for_memon_1_0_0() const {
        nlohmann::ordered_json json_metadata;
        if (not title.empty()) {
            json_metadata["title"] = title;
        }
        if (not artist.empty()) {
            json_metadata["artist"] = artist;
        }
        if (not audio.empty()) {
            json_metadata["audio"] = audio;
        }
        if (not jacket.empty()) {
            json_metadata["jacket"] = jacket;
        }
        if (use_preview_file) {
            if (not preview_file.empty()) {
                json_metadata["preview"] = preview_file;
            }
        } else {
            if (preview_loop.duration != 0) {
                json_metadata["preview"] = {
                    {"start", preview_loop.start.format("f")},
                    {"duration", preview_loop.duration.format("f")}
                };
            }
        }
        return json_metadata;
    };
}