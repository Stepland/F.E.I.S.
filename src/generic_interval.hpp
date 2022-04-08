#pragma once

#include <algorithm>
#include <compare>

#include <SFML/System/Time.hpp>

template<class A>
class Interval {
public:
    Interval() = default;
    Interval(const A& start, const A& end) :
        start(std::min(start, end)),
        end(std::max(start, end))
    {};

    bool operator==(const Interval&) const = default;

    bool intersects(const Interval<A>& other) const {
        return start <= other.end and end >= other.start;
    }

    A width() const {
        return end - start;
    }

    // interval union
    Interval<A>& operator+=(const Interval<A>& rhs) {
        start = std::min(start, rhs.start);
        end = std::max(end, rhs.end);
        return *this;
    };

    Interval& operator+=(const A& rhs) {
        return this->operator+=(Interval{rhs, rhs});
    };

    // passing lhs by value helps optimize chained a+b+c (says cppreference)
    template<class B>
    friend Interval operator+(Interval<A> lhs, const B& rhs) {
        lhs += rhs;
        return lhs;
    };

    A start;
    A end;
};