#pragma once

#include <memory>
#include <optional>
#include <set>

#include <json.hpp>

#include "better_hakus.hpp"
#include "better_notes.hpp"
#include "better_timing.hpp"
#include "special_numeric_types.hpp"

namespace better {
    struct Chart {
        std::optional<Decimal> level;
        Timing timing;
        std::optional<Hakus> hakus;
        Notes notes;

        nlohmann::ordered_json dump_for_memon_1_0_0(
            const nlohmann::ordered_json& fallback_timing_object
        ) const;
    };

    /*
    Create a brand new json based on 'object' but with keys identical to
    'fallback' removed
    */
    nlohmann::ordered_json remove_common_keys(
        const nlohmann::ordered_json& object,
        const nlohmann::ordered_json& fallback
    );

    nlohmann::ordered_json dump_memon_1_0_0_timing_object(
        const Timing& timing,
        const std::optional<Hakus>& hakus,
        const nlohmann::ordered_json& fallback_timing_object
    );
}