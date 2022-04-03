#pragma once

#include <compare>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>

#include <json.hpp>

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
        explicit Position(std::uint64_t index);
        Position(std::uint64_t x, std::uint64_t y);

        std::uint64_t index() const;
        std::uint64_t get_x() const;
        std::uint64_t get_y() const;

        auto operator<=>(const Position&) const = default;

    private:
        std::uint64_t x;
        std::uint64_t y;
    };

    std::ostream& operator<<(std::ostream& out, const Position& pos);

    class TapNote {
    public:
        TapNote(Fraction time, Position position);
        Fraction get_time() const;
        Position get_position() const;

        bool operator==(const TapNote&) const = default;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;
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
        std::uint64_t get_tail_length() const;
        std::uint64_t get_tail_angle() const;

        bool operator==(const LongNote&) const = default;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;
        int tail_as_6_notation() const;
    private:
        Fraction time;
        Position position;
        Fraction duration;
        Position tail_tip;
    };

    Position convert_legacy_memon_tail_index_to_position(const Position& pos, std::uint64_t tail_index);
    Position convert_6_notation_to_position(const Position& pos, std::uint64_t tail_index);

    class Note {
    public:
        template<typename ...Ts>
        Note(Ts&&... Args) : note(std::forward<Ts...>(Args...)) {};
        Fraction get_time() const;
        std::pair<Fraction, Fraction> get_time_bounds() const;
        Position get_position() const;
        Fraction get_end() const;

        template<typename T>
        auto visit(T& visitor) const {return std::visit(visitor, this->note);};

        bool operator==(const Note&) const = default;

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Note load_from_memon_0_1_0(
            const nlohmann::json& json,
            std::uint64_t resolution
        );

        static Note load_from_memon_legacy(
            const nlohmann::json& json,
            std::uint64_t resolution
        );
    private:
        std::variant<TapNote, LongNote> note;
    };
}

