#include "better_song.hpp"

namespace better {
    std::optional<sf::Time> Chart::time_of_last_event() const {
        if (notes.empty()) {
            return {};
        } else {
            return timing.time_at(notes.crbegin()->second.get_end());
        }
    };

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
}