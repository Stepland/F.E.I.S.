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
        std::optional<Timing> timing;
        std::optional<Hakus> hakus;
        Notes notes;

        nlohmann::ordered_json dump_for_memon_1_0_0() const;
    };
}