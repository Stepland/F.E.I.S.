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
        friend std::ostream& operator<<(std::ostream& out, const BPMEvent& b);
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

        using events_by_beats_type = std::set<BPMEvent, OrderByBeats>;
        using events_by_seconds_type = std::set<BPMEvent, OrderBySeconds>;

        Timing();
        Timing(const std::vector<BPMAtBeat>& events, const Decimal& offset);

        double seconds_at(Fraction beats) const;
        double seconds_between(Fraction beat_a, Fraction beat_b) const;
        sf::Time time_at(Fraction beats) const;
        sf::Time time_between(Fraction beat_a, Fraction beat_b) const;

        Fraction beats_at(sf::Time time) const;
        Fraction beats_at(double seconds) const;

        Decimal bpm_at(sf::Time time) const;
        Decimal bpm_at(double seconds) const;
        Decimal bpm_at(Fraction beats) const;

        void insert(const BPMAtBeat& bpm_change);

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Timing load_from_memon_1_0_0(const nlohmann::json& json);
        static Timing load_from_memon_legacy(const nlohmann::json& metadata);

        const events_by_beats_type& get_events_by_beats() const;

        bool operator==(const Timing&) const = default;

        friend std::ostream& operator<<(std::ostream& out, const Timing& t);
        friend fmt::formatter<better::Timing>;
    private:
        Decimal offset;
        double offset_as_double; 
        events_by_beats_type events_by_beats;
        events_by_seconds_type events_by_seconds;

        void reload_events_from(const std::vector<BPMAtBeat>& events);

        const BPMEvent& bpm_event_in_effect_at(sf::Time time) const;
        const BPMEvent& bpm_event_in_effect_at(double seconds) const;
        const BPMEvent& bpm_event_in_effect_at(Fraction beats) const;

        events_by_seconds_type::iterator iterator_to_bpm_event_in_effect_at(sf::Time time) const;
        events_by_seconds_type::iterator iterator_to_bpm_event_in_effect_at(double seconds) const;
        events_by_beats_type::iterator iterator_to_bpm_event_in_effect_at(Fraction beats) const;
    };
}

template <>
struct fmt::formatter<better::BPMEvent>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::BPMEvent& b, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "BPMEvent(beats: {}, bpm: {})",
            b.get_beats(),
            b.get_bpm()
        );
    }
};

template <>
struct fmt::formatter<better::Timing>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::Timing& t, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "Timing(offset: {}, events: [{}])",
            t.offset,
            fmt::join(t.events_by_beats, ", ")
        );
    }
};