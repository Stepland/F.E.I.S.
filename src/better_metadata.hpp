#pragma once

#include <string>

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
    };
}