#pragma once

#include <SFML/Config.hpp>
#include <algorithm>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>

#include <SFML/System/Time.hpp>

#include "special_numeric_types.hpp"

namespace better {
    struct SecondsAtBeat {
        Fraction seconds;
        Fraction beats;
    };

    class BPMAtBeat {
    public:
        BPMAtBeat(Fraction beats, Fraction bpm);
        Fraction get_beats() const;
        Fraction get_bpm() const;
    private:
        Fraction beats;
        Fraction bpm;
    };

    class BPMEvent : public BPMAtBeat {
    public:
        BPMEvent(Fraction beats, Fraction seconds, Fraction bpm);
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
        Timing(const std::vector<BPMAtBeat>& events, const SecondsAtBeat& offset);

        Fraction fractional_seconds_at(Fraction beats) const;
        sf::Time time_at(Fraction beats) const;

    private:
        std::set<BPMEvent, decltype(order_by_beats)> events_by_beats{order_by_beats};
        std::set<BPMEvent, decltype(order_by_seconds)> events_by_seconds{order_by_seconds};
    };
}