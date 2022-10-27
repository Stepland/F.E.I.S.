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

    class SelectableBPMEvent {
    public:
        SelectableBPMEvent(Fraction beats, double seconds, Decimal bpm);
        Decimal get_bpm() const;
        double get_bpm_as_double() const;
        Fraction get_beats() const;
        double get_seconds() const;

        bool operator==(const SelectableBPMEvent&) const = default;
        friend std::ostream& operator<<(std::ostream& out, const SelectableBPMEvent& b);

        mutable bool selected = false;
    private:
        Decimal bpm;
        double bpm_as_double;
        Fraction beats;
        double seconds;
    };

    class Timing {
    public:

        using event_type = SelectableBPMEvent;
        using key_type = std::shared_ptr<event_type>;

        struct OrderByBeats {
            template<class T>
            bool operator()(const T& a, const T& b) const {
                return a->get_beats() < b->get_beats();
            }
        };
        
        struct OrderBySeconds {
            template<class T>
            bool operator()(const T& a, const T& b) const {
                return a->get_seconds() < b->get_seconds();
            }
        };

        using events_by_beats_type = std::set<key_type, OrderByBeats>;
        using events_by_seconds_type = std::set<key_type, OrderBySeconds>;

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
        
        template<typename Callback>
        void for_each_event_between(const Fraction& first, const Fraction& last, const Callback& cb) const {
            const auto first_element = events_by_beats.lower_bound(
                std::make_shared<event_type>(first, 0, 1)
            );
            const auto last_element = events_by_beats.upper_bound(
                std::make_shared<event_type>(last, 0, 1)
            );
            std::for_each(first_element, last_element, [&](const key_type& ptr){cb(*ptr);});
        }

        void unselect_everything() const;

        bool operator==(const Timing&) const = default;

        friend std::ostream& operator<<(std::ostream& out, const Timing& t);
        friend fmt::formatter<better::Timing>;
    private:
        Decimal offset;
        double offset_as_double;

        // These containers hold shared pointers to the same objects
        events_by_beats_type events_by_beats;
        events_by_seconds_type events_by_seconds;

        void reload_events_from(const std::vector<BPMAtBeat>& events);

        const key_type& bpm_event_in_effect_at(sf::Time time) const;
        const key_type& bpm_event_in_effect_at(double seconds) const;
        const key_type& bpm_event_in_effect_at(Fraction beats) const;

        events_by_seconds_type::iterator iterator_to_bpm_event_in_effect_at(sf::Time time) const;
        events_by_seconds_type::iterator iterator_to_bpm_event_in_effect_at(double seconds) const;
        events_by_beats_type::iterator iterator_to_bpm_event_in_effect_at(Fraction beats) const;
    };
}

template <>
struct fmt::formatter<better::SelectableBPMEvent>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::SelectableBPMEvent& b, FormatContext& ctx) {
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