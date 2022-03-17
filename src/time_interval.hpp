#pragma once

#include <compare>

#include <SFML/System/Time.hpp>

class TimeInterval {
public:
    TimeInterval() = default;
    TimeInterval(const sf::Time& start, const sf::Time& end);

    bool operator==(const TimeInterval&) const = default;

    TimeInterval& operator+=(const TimeInterval& rhs);
    TimeInterval& operator+=(const sf::Time& rhs);

    // passing lhs by value helps optimize chained a+b+c (says cppreference)
    template<typename T>
    friend TimeInterval operator+(TimeInterval lhs, const T& rhs) {
        lhs += rhs;
        return lhs;
    };

    sf::Time start;
    sf::Time end;
};