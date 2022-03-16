#include "better_note.hpp"

namespace better {
    Position::Position(unsigned int index) : x(index % 4), y (index / 4) {
        if (index > 15) {
            std::stringstream ss;
            ss << "Attempted to create Position from invalid index : " << index; 
            throw std::invalid_argument(ss.str());
        }
    };

    Position::Position(unsigned int x, unsigned int y) : x(x), y(y) {
        if (x > 3 or y > 3) {
            std::stringstream ss;
            ss << "Attempted to create Position from invalid coordinates : ";
            ss << this;
            throw std::invalid_argument(ss.str()); 
        }
    };

    unsigned int Position::index() const {
        return x + y*4;
    };

    unsigned int Position::get_x() const {
        return x;
    };

    unsigned int Position::get_y() const {
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
            ss << duration;
            throw std::invalid_argument(ss.str());
        }
        if (tail_tip.get_x() != position.get_x() and tail_tip.get_y() != position.get_y()) {
            std::stringstream ss;
            ss << "Attempted to create a LongNote with invalid tail tip : ";
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
    
    auto _time_bounds = VariantVisitor {
        [](const TapNote& t) -> std::pair<Fraction, Fraction> { return {t.get_time(), t.get_time()}; },
        [](const LongNote& l) -> std::pair<Fraction, Fraction> { return {l.get_time(), l.get_end()}; },
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
}

