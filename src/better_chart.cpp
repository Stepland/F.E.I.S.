#include "better_chart.hpp"

#include <json.hpp>

namespace better {
    nlohmann::ordered_json Chart::dump_for_memon_1_0_0() const {
        nlohmann::ordered_json json_chart;
        if (level) {
            json_chart["level"] = level->format("f");
        }
        if (timing) {
            json_chart["timing"] = timing->dump_for_memon_1_0_0();
        }
        json_chart["notes"] = notes.dump_for_memon_1_0_0();
    }
}