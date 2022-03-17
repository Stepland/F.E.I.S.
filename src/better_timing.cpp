#include "better_timing.hpp"

namespace better {
    BPMAtBeat::BPMAtBeat(Fraction beats, Fraction bpm) : beats(beats), bpm(bpm) {
        if (bpm <= 0) {
            std::stringstream ss;
            ss << "Attempted to create a BPMAtBeat with negative BPM : ";
            ss << bpm;
            throw std::invalid_argument(ss.str());
        }
    };

    BPMEvent::BPMEvent(Fraction beats, Fraction seconds, Fraction bpm) :
        BPMAtBeat(beats, bpm),
        seconds(seconds)
    {};

    Fraction BPMEvent::get_seconds() const {
        return seconds;
    };
    
    /*
    Create a Time Map from a list of BPM changes with times given in
    beats, the offset parameter is more flexible than a "regular" beat zero
    offset as it accepts non-zero beats
    */
    Timing::Timing(const std::vector<BPMAtBeat>& events, const SecondsAtBeat& offset) {
        if (events.empty()) {
            throw std::invalid_argument(
                "Attempted to create a Timing object with no BPM events"
            );
        }

        std::multiset<
            BPMAtBeat,
            decltype(order_by_beats)
        > grouped_by_beats{
            events.begin(), events.end(), order_by_beats
        };

        std::set<BPMAtBeat, decltype(order_by_beats)> sorted_events{
            events.begin(), events.end(), order_by_beats
        };

        for (const auto& bpm_at_beat : sorted_events) {
            auto [begin, end] = grouped_by_beats.equal_range(bpm_at_beat);
            if (std::distance(begin, end) > 1) {
                std::stringstream ss;
                ss << "Attempted to create a Timing object with multiple ";
                ss << "BPMs defined at beat " << bpm_at_beat.get_beats();
                ss << " :";
                std::for_each(begin, end, [&ss](auto b){
                    ss << " (bpm: " << b.get_bpm() << ", beat: ";
                    ss << b.get_beats() << "),";
                });
                throw std::invalid_argument(ss.str());
            }
        }

        // First compute everything as if the first BPM change happened at
        // zero seconds, then shift according to the offset
        auto first_event = sorted_events.begin();
        Fraction current_second = 0;
        std::vector<BPMEvent> bpm_changes;
        bpm_changes.reserve(sorted_events.size());
        bpm_changes.emplace_back(
            first_event->get_beats(),
            current_second,
            first_event->get_bpm()
        );

        auto previous = first_event;
        auto current = std::next(first_event);
        for (; current != sorted_events.end(); ++previous, ++current) {
            auto beats_since_last_event =
                current->get_beats() - previous->get_beats();
            auto seconds_since_last_event = 
                (60 * beats_since_last_event) / previous->get_bpm();
            current_second += seconds_since_last_event;
            bpm_changes.emplace_back(
                current->get_beats(),
                current_second,
                current->get_bpm()
            );
        }
        this->events_by_beats
            .insert(bpm_changes.begin(), bpm_changes.end());
        this->events_by_seconds
            .insert(bpm_changes.begin(), bpm_changes.end());
        auto unshifted_seconds_at_offset =
            this->fractional_seconds_at(offset.beats);
        auto shift = offset.seconds - unshifted_seconds_at_offset;
        std::vector<BPMEvent> shifted_bpm_changes;
        std::transform(
            bpm_changes.begin(),
            bpm_changes.end(),
            std::back_inserter(shifted_bpm_changes),
            [shift](const BPMEvent& b){
                return BPMEvent(
                    b.get_beats(),
                    b.get_seconds() + shift,
                    b.get_bpm()
                );
            }
        );
        this->events_by_beats
            .insert(shifted_bpm_changes.begin(), shifted_bpm_changes.end());
        this->events_by_seconds
            .insert(shifted_bpm_changes.begin(), shifted_bpm_changes.end());
    }

    /*
    Return the number of seconds at the given beat.

    Before the first bpm change, compute backwards from the first bpm,
    after the first bpm change, compute forwards from the previous bpm
    change
    */
    Fraction Timing::fractional_seconds_at(Fraction beats) const {
        auto bpm_change = this->events_by_beats.upper_bound(BPMEvent(beats, 0, 0));
        if (bpm_change != this->events_by_beats.begin()) {
            bpm_change = std::prev(bpm_change);
        }
        auto beats_since_previous_event = beats - bpm_change->get_beats();
        auto seconds_since_previous_event = (
            Fraction{60}
            * beats_since_previous_event
            / bpm_change->get_bpm()
        );
        return bpm_change->get_seconds() + seconds_since_previous_event;
    };

    Fraction Timing::fractional_seconds_between(
        Fraction beat_a,
        Fraction beat_b
    ) const {
        return (
            fractional_seconds_at(beat_b)
            - fractional_seconds_at(beat_a)
        );
    };

    sf::Time Timing::time_at(Fraction beats) const {
        return frac_to_time(fractional_seconds_at(beats));
    };

    sf::Time Timing::time_between(Fraction beat_a, Fraction beat_b) const {
        return frac_to_time(fractional_seconds_between(beat_a, beat_b));
    };

    Fraction Timing::beats_at(sf::Time time) const {
        Fraction fractional_seconds{time.asMicroseconds(), 1000000};
        auto bpm_change = this->events_by_seconds.upper_bound(BPMEvent(0, fractional_seconds, 0));
        if (bpm_change != this->events_by_seconds.begin()) {
            bpm_change = std::prev(bpm_change);
        }
        auto seconds_since_previous_event = fractional_seconds - bpm_change->get_seconds();
        auto beats_since_previous_event = (
            bpm_change->get_bpm()
            * seconds_since_previous_event
            / Fraction{60}
        );
        return bpm_change->get_beats() + beats_since_previous_event;
    };
}