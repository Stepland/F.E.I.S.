#include "better_notes.hpp"

#include <SFML/System/Time.hpp>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include "fmt/core.h"
#include "json.hpp"

namespace better {
    std::pair<Notes::iterator, bool> Notes::insert(const Note& note) {
        auto conflicting_note = end();
        in(
            note.get_time_bounds(),
            [&](const Notes::iterator& it){
                if (
                    it->second.get_position() == note.get_position()
                    and conflicting_note == end()
                ) {
                    conflicting_note = it;
                }
            }
        );
        if (conflicting_note != end()) {
            return {conflicting_note, false};
        } else {
            auto it = interval_tree::insert({note.get_time_bounds(), note});
            return {it, true};
        }
    };

    Notes Notes::overwriting_insert(const Note& note) {
        Notes erased;
        in(
            note.get_time_bounds(),
            [&](const Notes::const_iterator& it) {
                if (it->second.get_position() == note.get_position()) {
                    erased.insert(it->second);
                }
            }
        );
        for (const auto& [_, conflict] : erased) {
            erase(conflict);
        }
        interval_tree::insert({note.get_time_bounds(), note});
        return erased;
    };

    void Notes::merge(Notes&& other) {
        for (auto&& [_, note]: other) {
            insert(std::move(note));
        }
    }

    Notes::const_iterator Notes::find(const Note& note) const {
        auto conflicting_note = interval_tree::end();
        in(
            note.get_time_bounds(),
            [&](Notes::const_iterator it){
                if (it->second == note and conflicting_note == end()) {
                    conflicting_note = it;
                }
            }
        );
        return conflicting_note;
    };

    bool Notes::contains(const Note& note) const {
        return find(note) != cend();
    };

    void Notes::erase(const Note& note) {
        auto it = find(note);
        interval_tree::erase(it);
    };


    bool Notes::is_colliding(const better::Note& note, const better::Timing& timing) const {
        const auto [start_beat, end_beat] = note.get_time_bounds();

        /*
        Two notes collide if they are within ~one second of each other :
        Approach and burst animations of original jubeat markers last 16 frames
        at (supposedly) 30 fps, which means a note needs (a bit more than) half
        a second of leeway both before *and* after itself to properly display 
        its marker animation, consequently, two consecutive notes on the same
        button cannot be closer than ~one second from each other.

        I don't really know why I shrink the collision zone down here ?
        Shouldn't it be 32/30 seconds ? (~1066ms instead of 1000ms ?)

        Reverse-engineering of the jubeat plus iOS app suggest the "official"
        note collision zone size is 1030 ms, so actually I wasn't that far off
        with 1000 ms !

        TODO: Make the collision zone customizable
        */
        const auto collision_start = timing.beats_at(timing.time_at(start_beat) - sf::seconds(1));
        const auto collision_end = timing.beats_at(timing.time_at(end_beat) + sf::seconds(1));

        bool found_collision = false;
        in(
            {collision_start, collision_end},
            [&](const Notes::const_iterator& it){
                if (it->second.get_position() == note.get_position()) {
                    if (it->second != note) {
                        found_collision = true;
                    }
                }
            }
        );
        return found_collision;
    };

    Notes Notes::between(const Interval<Fraction>& bounds) {
        Notes res;
        in(bounds.start, bounds.end, [&](const Notes::const_iterator& it){
            res.insert(it->second);
        });
        return res;
    };

    std::size_t Notes::count_between(const Interval<Fraction>& bounds) {
        std::size_t count = 0;
        in(
            {bounds.start, bounds.end},
            [&](const Notes::const_iterator& it){count++;}
        );
        return count;
    };

    nlohmann::ordered_json Notes::dump_to_memon_1_0_0() const {
        auto json_notes = nlohmann::ordered_json::array();
        for (const auto& [_, note] : *this) {
            json_notes.push_back(note.dump_to_memon_1_0_0());
        }
        return json_notes;
    };

    Notes Notes::load_from_memon_1_0_0(const nlohmann::json& json, std::uint64_t resolution) {
        Notes notes;
        for (auto& json_note : json) {
            try {
                const auto note = Note::load_from_memon_1_0_0(json_note, resolution);
                notes.insert(note);
            } catch (const std::exception&) {
                continue;
            }
        }
        return notes;
    };

    Notes Notes::load_from_memon_legacy(const nlohmann::json& json, std::uint64_t resolution) {
        Notes notes;
        for (auto& json_note : json) {
            try {
                const auto note = Note::load_from_memon_legacy(json_note, resolution);
                notes.insert(note);
            } catch (const std::exception&) {
                continue;
            }
        }
        return notes;
    };

    std::ostream& operator<<(std::ostream& out, const Notes& ns) {
        out << fmt::format("{}", ns);
        return out;
    };
}