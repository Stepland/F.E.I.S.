#include "better_hakus.hpp"
#include <algorithm>

#include "better_beats.hpp"

nlohmann::ordered_json dump_hakus(const Hakus& hakus) {
    auto j = nlohmann::ordered_json::array();
    for (const auto& haku : hakus) {
        j.push_back(beat_to_best_form(haku));
    }
    return j;
}

std::optional<Hakus> load_hakus(const nlohmann::json& timing) {
    if (not timing.contains("hakus")) {
        return {};
    }
    std::uint64_t resolution = 240;
    if (timing.contains("resolution")) {
        resolution = timing["resolution"].get<std::uint64_t>();
    }
    Hakus hakus;
    for (const auto& beat_json: timing["hakus"]) {
        hakus.insert(load_memon_1_0_0_beat(beat_json, resolution));
    }
    return hakus;
}