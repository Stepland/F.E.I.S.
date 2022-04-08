#include "better_note.hpp"

#include <variant>

#include "better_beats.hpp"

namespace better {
    Position::Position(std::uint64_t index) : x(index % 4), y (index / 4) {
        if (index > 15) {
            std::stringstream ss;
            ss << "Attempted to create Position from invalid index : " << index; 
            throw std::invalid_argument(ss.str());
        }
    };

    Position::Position(std::uint64_t x, std::uint64_t y) : x(x), y(y) {
        if (x > 3 or y > 3) {
            std::stringstream ss;
            ss << "Attempted to create Position from invalid coordinates : ";
            ss << this;
            throw std::invalid_argument(ss.str()); 
        }
    };

    std::uint64_t Position::index() const {
        return x + y*4;
    };

    std::uint64_t Position::get_x() const {
        return x;
    };

    std::uint64_t Position::get_y() const {
        return y;
    };

    std::ostream& operator<< (std::ostream& out, const Position& pos) {
        out << "(x: " << pos.get_x() << ", y: " << pos.get_y() << ")";
        return out;
    };


    TapNote::TapNote(Fraction time, Position position): time(time), position(position) {};

    Fraction TapNote::get_time() const {
        return time;
    };

    Position TapNote::get_position() const {
        return position;
    };

    nlohmann::ordered_json TapNote::dump_to_memon_1_0_0() const {
        return {
            {"n", position.index()},
            {"t", beat_to_best_form(time)}
        };
    };

    LongNote::LongNote(Fraction time, Position position, Fraction duration, Position tail_tip)
    :
        time(time),
        position(position),
        duration(duration),
        tail_tip(tail_tip)
    {
        if (duration < 0) {
            std::stringstream ss;
            ss << "Attempted to create a LongNote with negative duration : ";
            ss << duration.get_str();
            throw std::invalid_argument(ss.str());
        }
        if (tail_tip == position) {
            throw std::invalid_argument(
                "Attempted to create a LongNote with a zero-length tail"
            );
        }
        if (
            not (
                (tail_tip.get_x() == position.get_x())
                xor (tail_tip.get_y() == position.get_y())
            )
        ) {
            std::stringstream ss;
            ss << "Attempted to create a LongNote with and invalid tail : ";
            ss << "position: " << position << " , tail_tip: " << tail_tip;
            throw std::invalid_argument(ss.str());
        }
    };

    Fraction LongNote::get_time() const {
        return time;
    };

    Position LongNote::get_position() const {
        return position;
    };

    Fraction LongNote::get_end() const {
        return time + duration;
    };

    Fraction LongNote::get_duration() const {
        return duration;
    };

    Position LongNote::get_tail_tip() const {
        return tail_tip;
    };

    const auto abs_diff = [](const auto& a, const auto& b){
        if (a > b) {
            return a - b;
        } else {
            return b - a;
        }
    };

    std::uint64_t LongNote::get_tail_length() const {
        if (position.get_x() == tail_tip.get_x()) {
            return abs_diff(position.get_y(), tail_tip.get_y());
        } else {
            return abs_diff(position.get_x(), tail_tip.get_x());
        }
    }

    std::uint64_t LongNote::get_tail_angle() const {
        if (position.get_x() == tail_tip.get_x()) {
            if (position.get_y() > tail_tip.get_y()) {
                return 0;
            } else {
                return 180;
            }
        } else {
            if (position.get_x() > tail_tip.get_x()) {
                return 270;
            } else {
                return 90;
            }
        }
    };

    nlohmann::ordered_json LongNote::dump_to_memon_1_0_0() const {
        return {
            {"n", position.index()},
            {"t", beat_to_best_form(time)},
            {"l", beat_to_best_form(duration)},
            {"p", tail_as_6_notation()}
        };
    };

    int LongNote::tail_as_6_notation() const {
        if (tail_tip.get_y() == position.get_y()) {
            return tail_tip.get_x() - static_cast<int>(tail_tip.get_x() > position.get_x());
        } else {
            return 3 + tail_tip.get_y() - int(tail_tip.get_y() > position.get_y());
        }
    }

    /*
     *  legacy long note tail index is given relative to the note position :
     *  
     *          8
     *          4
     *          0
     *   11 7 3 . 1 5 9
     *          2
     *          6
     *         10
     */
    Position convert_legacy_memon_tail_index_to_position(const Position& pos, std::uint64_t tail_index) {
        auto length = (tail_index / 4) + 1;
        switch (tail_index % 4) {
            case 0: // up
                return {pos.get_x(), pos.get_y() - length};
            case 1: // right
                return {pos.get_x() + length, pos.get_y()};
            case 2: // down
                return {pos.get_x(), pos.get_y() + length};
            case 3: // left
                return {pos.get_x() - length, pos.get_y()};
            default:
                return pos;
        }
    }

    /*
    memon 1.0.0 stores tail positions as an number between 0 and 5 inclusive.
    
    Explainations here :
    https://memon-spec.readthedocs.io/en/latest/schema.html#long-note
    */
    Position convert_6_notation_to_position(const Position& pos, std::uint64_t tail_index) {
        if (tail_index < 3) {  // horizontal
            if (tail_index < pos.get_x()) {
                return {tail_index, pos.get_y()};
            } else {
                return {tail_index + 1, pos.get_y()};
            }
        } else {  // vertical
            tail_index -= 3;
            if (tail_index < pos.get_y()) {
                return {pos.get_x(), tail_index};
            } else {
                return {pos.get_x(), tail_index + 1};
            }
        }
    }
    
    auto _time_bounds = VariantVisitor {
        [](const TapNote& t) -> std::pair<Fraction, Fraction> { return {t.get_time(), t.get_time()}; },
        [](const LongNote& l) -> std::pair<Fraction, Fraction> { return {l.get_time(), l.get_end()}; },
    };

    Fraction Note::get_time() const {
        return std::visit([](const auto& n){return n.get_time();}, this->note);
    };

    std::pair<Fraction, Fraction> Note::get_time_bounds() const {
        return std::visit(better::_time_bounds, this->note);
    };

    Position Note::get_position() const {
        return std::visit([](const auto& n){return n.get_position();}, this->note);
    };

    Fraction Note::get_end() const {
        return this->get_time_bounds().second;
    }

    nlohmann::ordered_json Note::dump_to_memon_1_0_0() const {
        return std::visit([](const auto& n){return n.dump_to_memon_1_0_0();}, this->note);
    }

    Note Note::load_from_memon_0_1_0(const nlohmann::json& json, std::uint64_t resolution) {
        const auto position = Position{json["n"].get<std::uint64_t>()};
        const auto time = load_memon_1_0_0_beat(json["t"], resolution);
        if (not json.contains("l")) {
            return TapNote{time, position};
        }
        
        const auto duration = load_memon_1_0_0_beat(json["l"], resolution);
        const auto tail_index = json["n"].get<std::uint64_t>();
        return LongNote{
            time,
            position,
            duration,
            convert_6_notation_to_position(position, tail_index)
        };
    }

    Note Note::load_from_memon_legacy(const nlohmann::json& json, std::uint64_t resolution) {
        const auto position = Position{json["n"].get<std::uint64_t>()};
        const auto time = Fraction{
            json["t"].get<std::uint64_t>(),
            resolution,
        };
        const auto duration = Fraction{
            json["l"].get<std::uint64_t>(),
            resolution,
        };
        const auto tail_index = json["n"].get<std::uint64_t>();
        if (duration > 0) {
            return LongNote{
                time,
                position,
                duration,
                convert_legacy_memon_tail_index_to_position(position, tail_index)
            };
        } else {
            return TapNote{time, position};
        }
    }
}

