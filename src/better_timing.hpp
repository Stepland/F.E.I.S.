#pragma once

#include <SFML/Config.hpp>
#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <json.hpp>
#include <SFML/System/Time.hpp>

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
        double get_bpm_as_double() const;
        Fraction get_beats() const;
    private:
        Decimal bpm;
        double bpm_as_double;
        Fraction beats;
    };

    class BPMEvent {
    public:
        BPMEvent(Fraction beats, double seconds, Decimal bpm);
        Decimal get_bpm() const;
        double get_bpm_as_double() const;
        Fraction get_beats() const;
        double get_seconds() const;

        bool operator==(const BPMEvent&) const = default;
    private:
        Decimal bpm;
        double bpm_as_double;
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

        bool operator==(const Timing&) const = default;

    private:
        Decimal offset;
        double offset_as_double;
        std::set<BPMEvent, OrderByBeats> events_by_beats;
        std::set<BPMEvent, OrderBySeconds> events_by_seconds;
    };
}

template <>
struct fmt::formatter<better::Chart>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::Chart& c, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "LongNote(level: {}, timing: {}, hakus: {}, notes: {})",
            c.level,
            c.timing,
            c.hakus,
            c.notes
        );
    }
};