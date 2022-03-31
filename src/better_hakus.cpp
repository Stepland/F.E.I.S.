#include "better_hakus.hpp"

#include "better_beats.hpp"

nlohmann::ordered_json dump_hakus(const Hakus& hakus) {
    auto j = nlohmann::ordered_json::array();
    for (const auto& haku : hakus) {
        j.push_back(beat_to_best_form(haku));
    }
    return j;
}