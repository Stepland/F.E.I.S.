#pragma once

#include <string>

#include <json.hpp>

#include "special_numeric_types.hpp"

namespace better {
    struct PreviewLoop {
        Decimal start = 0;
        Decimal duration = 0;

        bool operator==(const PreviewLoop&) const = default;
        friend std::ostream& operator<<(std::ostream& out, const PreviewLoop& p);
    };

    struct Metadata {
        std::string title = "";
        std::string artist = "";
        std::string audio = "";
        std::string jacket = "";
        PreviewLoop preview_loop;
        std::string preview_file = "";
        bool use_preview_file = false;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Metadata load_from_memon_1_0_0(const nlohmann::json& json);
        static Metadata load_from_memon_0_3_0(const nlohmann::json& json);
        static Metadata load_from_memon_0_2_0(const nlohmann::json& json);
        static Metadata load_from_memon_0_1_0(const nlohmann::json& json);
        static Metadata load_from_memon_legacy(const nlohmann::json& json);

        bool operator==(const Metadata&) const = default;
        friend std::ostream& operator<<(std::ostream& out, const Metadata& m);
    };
}

template <>
struct fmt::formatter<better::PreviewLoop>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::PreviewLoop& p, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "PreviewLoop(start: {}, duration: {})",
            p.start,
            p.duration
        );
    }
};

template <>
struct fmt::formatter<better::Metadata>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::Metadata& m, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "Metadata(title: \"{}\", artist: \"{}\", audio: \"{}\", jacket: \"{}\", preview_loop: {}, preview_file: \"{}\", use_preview_file: {})",
            m.title,
            m.artist,
            m.audio,
            m.jacket,
            m.preview_loop,
            m.preview_file,
            m.use_preview_file
        );
    }
};