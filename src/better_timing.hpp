#pragma once

#include <SFML/Config.hpp>
#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <type_traits>
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

    class Timing {
    public:

        using bpm_event_type = BPMEvent;
        using key_type = bpm_event_type;

        struct beat_order_for_events {
            template<class T>
            bool operator()(const T& a, const T& b) const {
                return a.get_beats() < b.get_beats();
            }
        };

        using keys_by_beats_type = std::set<bpm_event_type, beat_order_for_events>;

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
        void erase(const BPMAtBeat& bpm_change);

        Decimal get_offset() const;
        void set_offset(const Decimal& new_offset);

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Timing load_from_memon_1_0_0(const nlohmann::json& json);
        static Timing load_from_memon_legacy(const nlohmann::json& metadata);
        
        template<typename Callback>
        void for_each_event_between(const Fraction& first, const Fraction& last, const Callback& cb) const {
            const auto first_element = events_by_beats.lower_bound(bpm_event_type{first, 0, 1});
            const auto last_element = events_by_beats.upper_bound(bpm_event_type{last, 0, 1});
            std::for_each(first_element, last_element, [&](const bpm_event_type& ptr){cb(ptr);});
        }

        bool operator==(const Timing&) const = default;

        friend std::ostream& operator<<(std::ostream& out, const Timing& t);
        friend fmt::formatter<better::Timing>;
    private:
        Decimal offset = 0;
        double offset_as_double = 0;

        // These containers hold shared pointers to the same objects
        keys_by_beats_type events_by_beats = {};
        std::map<double, Fraction> seconds_to_beats = {};

        void reconstruct(const std::vector<BPMAtBeat>& events, const Decimal& offset);

        /* Reload the timing object assuming the first event in the given
        vector happens at second zero */
        void reload_events_from(const std::vector<BPMAtBeat>& events);

        /* Shift all events in the timing object to make beat zero happen
        at the given offset in seconds */
        void shift_to_match(const Decimal& offset);

        const key_type& bpm_event_in_effect_at(sf::Time time) const;
        const key_type& bpm_event_in_effect_at(double seconds) const;
        const key_type& bpm_event_in_effect_at(Fraction beats) const;
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