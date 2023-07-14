#include "better_note.hpp"

#include <variant>

#include <fmt/core.h>

#include "better_beats.hpp"
#include "special_numeric_types.hpp"

namespace better {
    Position::Position(std::uint64_t x, std::uint64_t y) : x(x), y(y) {
        if (x > 3 or y > 3) {
            std::stringstream ss;
            ss << "Attempted to create Position from invalid coordinates : ";
            ss << *this;
            throw std::invalid_argument(ss.str()); 
        }
    };


    Position Position::from_index(std::uint64_t index) {
        if (index > 15) {
            std::stringstream ss;
            ss << "Attempted to create Position from invalid index : " << index; 
            throw std::invalid_argument(ss.str());
        }
        return Position{index % 4, index / 4};
    }

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
        out << fmt::format("{}", pos);
        return out;
    };

    Position Position::mirror_horizontally() const {
        return {3-x, y};
    }

    Position Position::mirror_vertically() const {
        return {x, 3-y};
    }

    Position Position::rotate_90_clockwise() const {
        return {3-y, x};
    }

    Position Position::rotate_90_counter_clockwise() const {
        return {y, 3-x};
    }

    Position Position::rotate_180() const {
        return {3-x, 3-y};
    }


    TapNote::TapNote(Fraction time, Position position): time(time), position(position) {};

    Fraction TapNote::get_time() const {
        return time;
    };

    Position TapNote::get_position() const {
        return position;
    };

    std::ostream& operator<<(std::ostream& out, const TapNote& t) {
        out << fmt::format("{}", t);
        return out;
    };

    nlohmann::ordered_json TapNote::dump_to_memon_1_0_0() const {
        return {
            {"n", position.index()},
            {"t", beat_to_best_form(time)}
        };
    };

    TapNote TapNote::mirror_horizontally() const {
        return {time, position.mirror_horizontally()};
    }

    TapNote TapNote::mirror_vertically() const {
        return {time, position.mirror_vertically()};
    }

    TapNote TapNote::rotate_90_clockwise() const {
        return {time, position.rotate_90_clockwise()};
    }

    TapNote TapNote::rotate_90_counter_clockwise() const {
        return {time, position.rotate_90_counter_clockwise()};
    }

    TapNote TapNote::rotate_180() const {
        return {time, position.rotate_180()};
    }

    TapNote TapNote::quantize(unsigned int snap) const {
        return {round_beats(time, snap), position};
    }


    LongNote::LongNote(Fraction time, Position position, Fraction duration, Position tail_tip) :
        time(time),
        position(position),
        duration(duration),
        tail_tip(tail_tip)
    {
        if (duration <= 0) {
            std::stringstream ss;
            ss << "Attempted to create a LongNote with zero or negative duration : ";
            ss << duration;
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

    std::ostream& operator<<(std::ostream& out, const LongNote& l) {
        out << fmt::format("{}", l);
        return out;
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

    LongNote LongNote::mirror_horizontally() const {
        return {
            time,
            position.mirror_horizontally(),
            duration,
            tail_tip.mirror_horizontally()
        };
    }

    LongNote LongNote::mirror_vertically() const {
        return {
            time,
            position.mirror_vertically(),
            duration,
            tail_tip.mirror_vertically()
        };
    }

    LongNote LongNote::rotate_90_clockwise() const {
        return {
            time,
            position.rotate_90_clockwise(),
            duration,
            tail_tip.rotate_90_clockwise()
        };
    }

    LongNote LongNote::rotate_90_counter_clockwise() const {
        return {
            time,
            position.rotate_90_counter_clockwise(),
            duration,
            tail_tip.rotate_90_counter_clockwise()
        };
    }

    LongNote LongNote::rotate_180() const {
        return {
            time,
            position.rotate_180(),
            duration,
            tail_tip.rotate_180()
        };
    }

    LongNote LongNote::quantize(unsigned int snap) const {
        return {
            round_beats(time, snap),
            position,
            round_beats(duration, snap),
            tail_tip
        };
    };


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

    std::ostream& operator<<(std::ostream& out, const Note& n) {
        out << fmt::format("{}", n);
        return out;
    };

    nlohmann::ordered_json Note::dump_to_memon_1_0_0() const {
        return std::visit([](const auto& n){return n.dump_to_memon_1_0_0();}, this->note);
    }

    Note Note::load_from_memon_1_0_0(const nlohmann::json& json, std::uint64_t resolution) {
        const auto position = Position::from_index(json["n"].get<std::uint64_t>());
        const auto time = load_memon_1_0_0_beat(json["t"], resolution);
        if (not json.contains("l")) {
            return TapNote{time, position};
        }
        
        const auto duration = load_memon_1_0_0_beat(json["l"], resolution);
        const auto tail_index = json["p"].get<std::uint64_t>();
        return LongNote{
            time,
            position,
            duration,
            convert_6_notation_to_position(position, tail_index)
        };
    }

    Note Note::load_from_memon_legacy(const nlohmann::json& json, std::uint64_t resolution) {
        const auto position = Position::from_index(json["n"].get<std::uint64_t>());
        const auto time = Fraction{
            json["t"].get<std::uint64_t>(),
            resolution,
        };
        const auto duration = Fraction{
            json["l"].get<std::uint64_t>(),
            resolution,
        };
        const auto tail_index = json["p"].get<std::uint64_t>();
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

    Note Note::mirror_horizontally() const {
        return std::visit([](const auto& n) -> Note {return n.mirror_horizontally();}, this->note);
    }

    Note Note::mirror_vertically() const {
        return std::visit([](const auto& n) -> Note {return n.mirror_vertically();}, this->note);
    }

    Note Note::rotate_90_clockwise() const {
        return std::visit([](const auto& n) -> Note {return n.rotate_90_clockwise();}, this->note);
    }

    Note Note::rotate_90_counter_clockwise() const {
        return std::visit([](const auto& n) -> Note {return n.rotate_90_counter_clockwise();}, this->note);
    }

    Note Note::rotate_180() const {
        return std::visit([](const auto& n) -> Note {return n.rotate_180();}, this->note);
    }

    Note Note::quantize(unsigned int snap) const {
        return std::visit([=](const auto& n) -> Note {return n.quantize(snap);}, this->note);
    }
}

