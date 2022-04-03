#pragma once

#include <SFML/Config.hpp>
#include <algorithm>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>

#include <SFML/System/Time.hpp>

#include "json.hpp"
#include "special_numeric_types.hpp"

namespace better {
    struct SecondsAtBeat {
        Fraction seconds;
        Fraction beats;
    };

    class BPMAtBeat {
    public:
        BPMAtBeat(Decimal bpm, Fraction beats);
        Decimal get_bpm() const;
        Fraction get_beats() const;
    private:
        Decimal bpm;
        Fraction beats;
    };

    class BPMEvent : public BPMAtBeat {
    public:
        BPMEvent(Fraction beats, Fraction seconds, Decimal bpm);
        Fraction get_seconds() const;
    private:
        Fraction seconds;
    };

    const auto order_by_beats = [](const BPMAtBeat& a, const BPMAtBeat& b) {
        return a.get_beats() < b.get_beats();
    };

    const auto order_by_seconds = [](const BPMEvent& a, const BPMEvent& b) {
        return a.get_seconds() < b.get_seconds();
    };

    class Timing {
    public:
        Timing();
        Timing(const std::vector<BPMAtBeat>& events, const SecondsAtBeat& offset);

        Fraction fractional_seconds_at(Fraction beats) const;
        Fraction fractional_seconds_between(Fraction beat_a, Fraction beat_b) const;
        sf::Time time_at(Fraction beats) const;
        sf::Time time_between(Fraction beat_a, Fraction beat_b) const;

        Fraction beats_at(sf::Time time) const;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Timing load_from_memon_1_0_0(const nlohmann::json& json);
        static Timing load_from_memon_legacy(const nlohmann::json& metadata);
        
    private:
        std::set<BPMEvent, decltype(order_by_beats)> events_by_beats{order_by_beats};
        std::set<BPMEvent, decltype(order_by_seconds)> events_by_seconds{order_by_seconds};
    };

    const auto frac_to_time = [](const Fraction& f) {
        auto microseconds = f * 1000000;
        return sf::microseconds(microseconds.convert_to<sf::Int64>());
    };
}