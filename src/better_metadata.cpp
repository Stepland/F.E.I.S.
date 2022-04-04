#include "better_metadata.hpp"

namespace better {
    nlohmann::ordered_json Metadata::dump_to_memon_1_0_0() const {
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

    Metadata Metadata::load_from_memon_1_0_0(const nlohmann::json& json) {
        Metadata metadata;
        metadata.title = json.value("title", "");
        metadata.artist = json.value("artist", "");
        metadata.audio = json.value("audio", "");
        metadata.jacket = json.value("jacket", "");
        if (json.contains("preview")) {
            if (json["preview"].is_string()) {
                metadata.use_preview_file = true;
                json["preview"].get_to(metadata.preview_file);
            } else if (json["preview"].is_object()) {
                metadata.use_preview_file = false;
                metadata.preview_loop.start = Decimal{json["preview"]["start"].get<std::string>()};
                metadata.preview_loop.duration = Decimal{json["preview"]["duration"].get<std::string>()};
            }
        }
        return metadata;
    };

    Metadata Metadata::load_from_memon_0_3_0(const nlohmann::json& json) {
        Metadata metadata;
        json["song title"].get_to(metadata.title);
        json["artist"].get_to(metadata.artist);
        metadata.audio = json.value("music path", "");
        metadata.jacket = json.value("album cover path", "");
        if (json.contains("preview")) {
            metadata.use_preview_file = false;
            metadata.preview_loop.start = Decimal{json["preview"]["position"].get<std::string>()};
            metadata.preview_loop.duration = Decimal{json["preview"]["length"].get<std::string>()};
        }
        if (json.contains("preview path")) {
            metadata.use_preview_file = true;
            json["preview path"].get_to(metadata.preview_file);
        }
        return metadata;
    };

    Metadata Metadata::load_from_memon_0_2_0(const nlohmann::json& json) {
        Metadata metadata;
        json["song title"].get_to(metadata.title);
        json["artist"].get_to(metadata.artist);
        json["music path"].get_to(metadata.audio);
        json["album cover path"].get_to(metadata.jacket);
        if (json.contains("preview")) {
            metadata.use_preview_file = false;
            metadata.preview_loop.start = Decimal{json["preview"]["position"].get<std::string>()};
            metadata.preview_loop.duration = Decimal{json["preview"]["length"].get<std::string>()};
        }
        return metadata;
    };

    Metadata Metadata::load_from_memon_0_1_0(const nlohmann::json& json) {
        Metadata metadata;
        json["song title"].get_to(metadata.title);
        json["artist"].get_to(metadata.artist);
        json["music path"].get_to(metadata.audio);
        json["album cover path"].get_to(metadata.jacket);
        return metadata;
    };

    Metadata Metadata::load_from_memon_legacy(const nlohmann::json& json) {
        Metadata metadata;
        json["song title"].get_to(metadata.title);
        json["artist"].get_to(metadata.artist);
        json["music path"].get_to(metadata.audio);
        json["jacket path"].get_to(metadata.jacket);
        return metadata;
    }
}