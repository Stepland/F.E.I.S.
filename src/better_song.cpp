#include "better_song.hpp"

#include <SFML/System/Time.hpp>

#include "std_optional_extras.hpp"

namespace better {
    std::optional<sf::Time> Chart::time_of_last_event() const {
        if (notes.empty()) {
            return {};
        } else {
            return timing.time_at(notes.crbegin()->second.get_end());
        }
    };

    bool Chart::is_colliding(const better::Note& note) {
        const auto [start_beat, end_beat] = note.get_time_bounds();

        /*
        Two notes collide if they are within ~one second of each other :
        Approach and burst animations of original jubeat markers last 16 frames
        at (supposedly) 30 fps, which means a note needs (a bit more than) half
        a second of leeway both before *and* after itself, consequently, two
        consecutive notes on the same button cannot be closer than ~one second
        from each other.

        I don't really know why I shrink the collision zone down here ?
        Shouldn't it be 32/30 seconds ? (1.0666... seconds instead of 1 ?)

        TODO: Make the collision zone customizable
        */
        const auto collision_start = timing.beats_at(timing.time_at(start_beat) - sf::seconds(1));
        const auto collision_end = timing.beats_at(timing.time_at(end_beat) + sf::seconds(1));

        bool found_collision = false;
        notes.in(
            {collision_start, collision_end},
            [&](Notes::const_iterator it){
                if (it->second.get_position() == note.get_position()) {
                    if (it->second != note) {
                        found_collision = true;
                    }
                }
            }
        );
        return found_collision;
    }

    PreviewLoop::PreviewLoop(Decimal start, Decimal duration) :
        start(start),
        duration(duration)
    {
        if (start < 0) {
            std::stringstream ss;
            ss << "Attempted to create a PreviewLoop with negative start ";
            ss << "position : " << start;
            throw std::invalid_argument(ss.str());
        }
        
        if (duration < 0) {
            std::stringstream ss;
            ss << "Attempted to create a PreviewLoop with negative ";
            ss << "duration : " << duration;
            throw std::invalid_argument(ss.str());
        }
    };

    Decimal PreviewLoop::get_start() const {
        return start;
    };

    Decimal PreviewLoop::get_duration() const {
        return duration;
    };

    std::string stringify_level(std::optional<Decimal> level) {
        return stringify_or(level, "(no level defined)");
    };
}