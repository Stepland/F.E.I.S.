#pragma once

#include <memory>
#include <optional>
#include <set>

#include <fmt/core.h>
#include <json.hpp>

#include "better_hakus.hpp"
#include "better_notes.hpp"
#include "better_timing.hpp"
#include "special_numeric_types.hpp"
#include "toolbox.hpp"

namespace better {
    struct Chart {
        std::optional<Decimal> level;
        std::optional<std::shared_ptr<Timing>> timing;
        std::optional<Hakus> hakus;
        std::shared_ptr<Notes> notes = std::make_shared<Notes>();

        bool operator==(const Chart&) const = default;

        nlohmann::ordered_json dump_to_memon_1_0_0(
            const nlohmann::ordered_json& fallback_timing_object
        ) const;

        static Chart load_from_memon_1_0_0(const nlohmann::json& json, const nlohmann::json& fallback_timing);
        static Chart load_from_memon_legacy(const nlohmann::json& json);

        friend std::ostream& operator<<(std::ostream& out, const Chart& c);
    };

    /*
    Create a brand new json based on 'object' but with keys identical to
    'fallback' removed
    */
    nlohmann::ordered_json remove_keys_already_in_fallback(
        const nlohmann::ordered_json& object,
        const nlohmann::ordered_json& fallback
    );

    nlohmann::ordered_json dump_memon_1_0_0_timing_object(
        const std::optional<std::shared_ptr<better::Timing>>& timing,
        const std::optional<Hakus>& hakus,
        const nlohmann::ordered_json& fallback_timing_object
    );
}

template <>
struct fmt::formatter<better::Chart>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::Chart& c, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "Chart(level: {}, timing: {}, hakus: {}, notes: {})",
            c.level,
            c.timing,
            c.hakus,
            c.notes
        );
    }
};