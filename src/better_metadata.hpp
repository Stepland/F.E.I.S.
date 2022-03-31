#pragma once

#include <string>

#include <json.hpp>

#include "special_numeric_types.hpp"

namespace better {
    struct PreviewLoop {
        Decimal start = 0;
        Decimal duration = 0;
    };

    struct Metadata {
        std::string title = "";
        std::string artist = "";
        std::string audio = "";
        std::string jacket = "";
        PreviewLoop preview_loop;
        std::string preview_file = "";
        bool use_preview_file = false;

        nlohmann::ordered_json dump_for_memon_1_0_0() const;
    };
}