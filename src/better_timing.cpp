#include "better_timing.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <iterator>
#include <memory>
#include <string>

#include <fmt/core.h>
#include <json.hpp>

#include "better_beats.hpp"
#include "json_decimal_handling.hpp"
#include "toolbox.hpp"

namespace better {
    BPMAtBeat::BPMAtBeat(Decimal bpm_, Fraction beats_) :
        bpm(bpm_),
        bpm_as_double(std::stod(bpm_.format("f"))),
        beats(beats_)
    {
        if (bpm <= 0) {
            std::stringstream ss;
            ss << "Attempted to create a BPMAtBeat with negative BPM : ";
            ss << bpm;
            throw std::invalid_argument(ss.str());
        }
    };

    Decimal BPMAtBeat::get_bpm() const {
        return bpm;
    }

    double BPMAtBeat::get_bpm_as_double() const {
        return bpm_as_double;
    }
    
    Fraction BPMAtBeat::get_beats() const {
        return beats;
    }

    SelectableBPMEvent::SelectableBPMEvent(Fraction beats_, double seconds_, Decimal bpm_) :
        bpm(bpm_),
        bpm_as_double(std::stod(bpm_.format("f"))),
        beats(beats_),
        seconds(seconds_)
    {};

    Decimal SelectableBPMEvent::get_bpm() const {
        return bpm;
    }

    double SelectableBPMEvent::get_bpm_as_double() const {
        return bpm_as_double;
    }
    
    Fraction SelectableBPMEvent::get_beats() const {
        return beats;
    }

    double SelectableBPMEvent::get_seconds() const {
        return seconds;
    };

    std::ostream& operator<<(std::ostream& out, const SelectableBPMEvent& b) {
        out << fmt::format("{}", b);
        return out;
    }
    

    // Default constructor, used when creating a new song from scratch
    Timing::Timing() :
        Timing({{120, 0}}, 0)
    {};
    
    /*
    Create a Time Map from a list of BPM changes with times given in
    beats, the offset parameter is more flexible than a "regular" beat zero
    offset as it accepts non-zero beats
    */
    Timing::Timing(const std::vector<BPMAtBeat>& events, const Decimal& offset_) :
        offset(offset_),
        offset_as_double(std::stod(offset_.format("f")))
    {
        reload_events_from(events);
    }

    /*
    Return the amount of seconds at the given beat.

    Before the first bpm change, compute backwards from the first bpm,
    after the first bpm change, compute forwards from the previous bpm
    change
    */
    double Timing::seconds_at(Fraction beats) const {
        const auto& bpm_change = bpm_event_in_effect_at(beats);
        const Fraction beats_since_previous_event = beats - bpm_change->get_beats();
        double seconds_since_previous_event = static_cast<double>(beats_since_previous_event) * 60 / bpm_change->get_bpm_as_double();
        return (
            offset_as_double
            + bpm_change->get_seconds()
            + seconds_since_previous_event
        );
    };

    double Timing::seconds_between(Fraction beat_a, Fraction beat_b) const {
        return seconds_at(beat_b) - seconds_at(beat_a);
    };

    sf::Time Timing::time_at(Fraction beats) const {
        return sf::seconds(seconds_at(beats));
    };

    sf::Time Timing::time_between(Fraction beat_a, Fraction beat_b) const {
        return sf::seconds(seconds_between(beat_a, beat_b));
    };

    Fraction Timing::beats_at(sf::Time time) const {
        const auto seconds = static_cast<double>(time.asMicroseconds()) / 1000000.0;
        return beats_at(seconds);
    };

    Fraction Timing::beats_at(double seconds) const {
        const auto& bpm_change = bpm_event_in_effect_at(seconds);
        auto seconds_since_previous_event = seconds - bpm_change->get_seconds();
        auto beats_since_previous_event = (
            convert_to_fraction(bpm_change->get_bpm())
            * Fraction{seconds_since_previous_event}
            / 60
        );
        return bpm_change->get_beats() + beats_since_previous_event;
    };

    Decimal Timing::bpm_at(sf::Time time) const {
        const auto seconds = static_cast<double>(time.asMicroseconds()) / 1000000.0;
        return bpm_at(seconds);
    }

    Decimal Timing::bpm_at(double seconds) const {
        const auto& bpm_change = bpm_event_in_effect_at(seconds);
        return bpm_change->get_bpm();
    }

    Decimal Timing::bpm_at(Fraction beats) const {
        const auto& bpm_change = bpm_event_in_effect_at(beats);
        return bpm_change->get_bpm();
    }

    void Timing::insert(const BPMAtBeat& new_bpm) {
        std::vector<BPMAtBeat> new_events;
        new_events.reserve(events_by_beats.size() + 1);
        // put the new bpm in front so it's kept instead of existing events at the same beat
        new_events.push_back(new_bpm);
        std::transform(
            events_by_beats.begin(),
            events_by_beats.end(),
            std::back_inserter(new_events),
            [&](const key_type& bpm){ return BPMAtBeat{bpm->get_bpm(), bpm->get_beats()};}
        );
        reload_events_from(new_events);
    }


    nlohmann::ordered_json Timing::dump_to_memon_1_0_0() const {
        nlohmann::ordered_json j;
        j["offset"] = offset.format("f");
        auto bpms = nlohmann::ordered_json::array();
        for (const auto& bpm_change : events_by_beats) {
            bpms.push_back({
                {"beat", beat_to_best_form(bpm_change->get_beats())},
                {"bpm", bpm_change->get_bpm().format("f")}
            });
        }
        j["bpms"] = bpms;
        return j;
    };

    Timing Timing::load_from_memon_1_0_0(const nlohmann::json& json) {
        Decimal offset = 0;
        if (json.contains("offset")) {
            offset = load_as_decimal(json["offset"]);
        }
        std::uint64_t resolution = 240;
        if (json.contains("resolution")) {
            resolution = json["resolution"].get<std::uint64_t>();
        }
        std::vector<BPMAtBeat> bpms;
        if (not json.contains("bpms")) {
            bpms = {{120, 0}};
        } else {
            for (const auto& bpm_json : json["bpms"]) {
                try {
                    bpms.emplace_back(
                        load_as_decimal(bpm_json["bpm"]),
                        load_memon_1_0_0_beat(bpm_json["beat"], resolution)
                    );
                } catch (const std::exception&) {
                    continue;
                }
            }
        }
        return {
            bpms,
            offset
        };
    };

    /*
    In legacy memon, offset is the OPPOSITE of the time (in seconds) at which
    the first beat occurs in the music file
    */
    Timing Timing::load_from_memon_legacy(const nlohmann::json& metadata) {
        const auto bpm = load_as_decimal(metadata["BPM"]);
        const auto offset = load_as_decimal(metadata["offset"]);
        return Timing{{{bpm, 0}}, -1 * offset};
    };

    void Timing::unselect_everything() const {
        std::ranges::for_each(events_by_beats, [&](auto& event){
            event->selected = false;
        });
    }

    struct _OrderByBeatsDeref {
        template<class T>
        bool operator()(const T& a, const T& b) const {
            return a.get_beats() < b.get_beats();
        }
    };

    void Timing::reload_events_from(const std::vector<BPMAtBeat>& events) {
        if (events.empty()) {
            throw std::invalid_argument(
                "Attempted to create a Timing object with no BPM events"
            );
        }

        // Sort events by beat, keeping only the first in insertion order in
        // case there are multiple on a given beat
        std::set<BPMAtBeat, _OrderByBeatsDeref> sorted_events;
        for (const auto& event: events) {
            if (not sorted_events.contains(event)) {
                sorted_events.insert(event);
            }
        }

        // Only keep non-redundant bpm changes
        std::set<BPMAtBeat, _OrderByBeatsDeref> filtered_events;
        for (const auto& event : sorted_events) {
            if (filtered_events.empty()) {
                filtered_events.insert(event);
            } else {
                const auto& previous_event = filtered_events.crbegin();
                if (event.get_bpm() != previous_event->get_bpm()) {
                    filtered_events.insert(event);
                }
            }
        }

        auto first_event = filtered_events.begin();
        double current_second = 0;
        std::vector<SelectableBPMEvent> bpm_changes;
        bpm_changes.reserve(filtered_events.size());
        bpm_changes.emplace_back(
            first_event->get_beats(),
            current_second,
            first_event->get_bpm()
        );

        auto previous = first_event;
        auto current = std::next(first_event);
        for (; current != filtered_events.end(); ++previous, ++current) {
            const Fraction beats_since_last_event = current->get_beats() - previous->get_beats();
            double seconds_since_last_event = static_cast<double>(beats_since_last_event) * 60 / previous->get_bpm_as_double();
            current_second += seconds_since_last_event;
            bpm_changes.emplace_back(
                current->get_beats(),
                current_second,
                current->get_bpm()
            );
        }
        events_by_beats.clear();
        std::transform(
            bpm_changes.begin(),
            bpm_changes.end(),
            std::inserter(events_by_beats, events_by_beats.begin()),
            [](auto&& event){
                return std::make_shared<event_type>(std::move(event));
            }
        );
        events_by_seconds.clear();
        events_by_seconds.insert(events_by_beats.begin(), events_by_beats.end());
    }

    const Timing::key_type& Timing::bpm_event_in_effect_at(sf::Time time) const {
        return *iterator_to_bpm_event_in_effect_at(time);
    }

    const Timing::key_type& Timing::bpm_event_in_effect_at(double seconds) const {
        return *iterator_to_bpm_event_in_effect_at(seconds);
    }

    const Timing::key_type& Timing::bpm_event_in_effect_at(Fraction beats) const {
        return *iterator_to_bpm_event_in_effect_at(beats);
    }

    Timing::events_by_seconds_type::iterator Timing::iterator_to_bpm_event_in_effect_at(sf::Time time) const {
        const auto seconds = static_cast<double>(time.asMicroseconds()) / 1000000.0;
        return iterator_to_bpm_event_in_effect_at(seconds);
    }

    Timing::events_by_seconds_type::iterator Timing::iterator_to_bpm_event_in_effect_at(double seconds) const {
        auto bpm_change = this->events_by_seconds.upper_bound(
            std::make_shared<event_type>(0, seconds - offset_as_double, 0)
        );
        if (bpm_change != this->events_by_seconds.begin()) {
            bpm_change = std::prev(bpm_change);
        }
        return bpm_change;
    }

    Timing::events_by_beats_type::iterator Timing::iterator_to_bpm_event_in_effect_at(Fraction beats) const {
        auto bpm_change = this->events_by_beats.upper_bound(
            std::make_shared<event_type>(beats, 0, 0)
        );
        if (bpm_change != this->events_by_beats.begin()) {
            bpm_change = std::prev(bpm_change);
        }
        return bpm_change;
    }



    std::ostream& operator<<(std::ostream& out, const Timing& t) {
        out << fmt::format("{}", t);
        return out;
    };
}