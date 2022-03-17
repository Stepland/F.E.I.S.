#pragma once

#include <compare>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>

#include "special_numeric_types.hpp"
#include "variant_visitor.hpp"

namespace better {
    /*
    A specific square on the controller. (0, 0) is the top-left button, x
    goes right, y goes down.
            x →
            0 1 2 3
        y 0 □ □ □ □
        ↓ 1 □ □ □ □
          2 □ □ □ □
          3 □ □ □ □
    */
    class Position {
    public:
        explicit Position(unsigned int index);
        Position(unsigned int x, unsigned int y);

        unsigned int index() const;
        unsigned int get_x() const;
        unsigned int get_y() const;

        auto operator<=>(const Position&) const = default;

    private:
        unsigned int x;
        unsigned int y;
    };

    std::ostream& operator<<(std::ostream& out, const Position& pos);

    class TapNote {
    public:
        TapNote(Fraction time, Position position);
        Fraction get_time() const;
        Position get_position() const;
    private:
        Fraction time;
        Position position;
    };

    class LongNote {
    public:
        LongNote(Fraction time, Position position, Fraction duration, Position tail_tip);
        
        Fraction get_time() const;
        Position get_position() const;
        Fraction get_end() const;
        Fraction get_duration() const;
        Position get_tail_tip() const;
        unsigned int get_tail_length() const;
        unsigned int get_tail_angle() const;
    private:
        Fraction time;
        Position position;
        Fraction duration;
        Position tail_tip;
    };

    class Note {
    public:
        template<typename ...Ts>
        Note(Ts&&... Args) : note(std::forward<Ts...>(Args...)) {};
        std::pair<Fraction, Fraction> get_time_bounds() const;
        Position get_position() const;
        Fraction get_end() const;
    private:
        std::variant<TapNote, LongNote> note;
    };
}

