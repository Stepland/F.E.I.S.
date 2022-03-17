#include "time_interval.hpp"

#include <algorithm>

TimeInterval::TimeInterval(const sf::Time& start, const sf::Time& end) :
    start(std::min(start, end)),
    end(std::max(start, end))
{};

// interval union
TimeInterval& TimeInterval::operator+=(const TimeInterval& rhs) {
    start = std::min(start, rhs.start);
    end = std::max(end, rhs.end);
    return *this;
};

TimeInterval& TimeInterval::operator+=(const sf::Time& rhs) {
    return this->operator+=(TimeInterval{rhs, rhs});
};