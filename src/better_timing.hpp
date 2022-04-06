#pragma once

#include <SFML/Config.hpp>
#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <SFML/System/Time.hpp>

#include "json.hpp"
#include "special_numeric_types.hpp"

namespace better {
    struct SecondsAtBeat {
        Decimal seconds;
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

    class BPMEvent {
    public:
        BPMEvent(Fraction beats, double seconds, Decimal bpm);
        Decimal get_bpm() const;
        Fraction get_beats() const;
        Fraction get_seconds() const;
    private:
        Decimal bpm;
        Fraction beats;
        double seconds;
    };

    struct OrderByBeats {
        template<class T>
        bool operator()(const T& a, const T& b) const {
            return a.get_beats() < b.get_beats();
        }
    };
    
    struct OrderBySeconds {
        template<class T>
        bool operator()(const T& a, const T& b) const {
            return a.get_seconds() < b.get_seconds();
        }
    };

    class Timing {
    public:
        Timing();
        Timing(const std::vector<BPMAtBeat>& events, const Decimal& offset);

        double seconds_at(Fraction beats) const;
        double seconds_between(Fraction beat_a, Fraction beat_b) const;
        sf::Time time_at(Fraction beats) const;
        sf::Time time_between(Fraction beat_a, Fraction beat_b) const;

        Fraction beats_at(sf::Time time) const;
        Fraction beats_at(double seconds) const;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Timing load_from_memon_1_0_0(const nlohmann::json& json);
        static Timing load_from_memon_legacy(const nlohmann::json& metadata);

        Decimal offset;
        
    private:
        std::set<BPMEvent, OrderByBeats> events_by_beats;
        std::set<BPMEvent, OrderBySeconds> events_by_seconds;
    };
}