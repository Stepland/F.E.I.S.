#include "long_note_dummy.hpp"

better::LongNote make_playfield_long_note_dummy(
    const Fraction& current_beat,
    const TapNotePair& long_note_being_created,
    const Fraction& snap
) {
    const auto note = make_linear_view_long_note_dummy(long_note_being_created, snap);
    return better::LongNote{
        current_beat,
        note.get_position(),
        note.get_duration(),
        note.get_tail_tip()
    };
};

better::LongNote make_linear_view_long_note_dummy(
    const TapNotePair& long_note_being_created,
    const Fraction& snap
) {
    auto start_time = long_note_being_created.first.get_time();
    auto end_time = long_note_being_created.second.get_time();
    
    if (start_time > end_time) {
        std::swap(start_time, end_time);
    }
    const auto duration = std::max(snap, end_time - start_time);
    return better::LongNote(
        start_time,
        long_note_being_created.first.get_position(),
        duration,
        closest_tail_position(
            long_note_being_created.first.get_position(),
            long_note_being_created.second.get_position()
        )
    );
};

better::Position closest_tail_position(
    const better::Position& anchor,
    const better::Position& requested_tail
) {
    if (anchor == requested_tail) {
        return smallest_possible_tail(anchor);
    }

    const auto delta_x = static_cast<int>(requested_tail.get_x()) - static_cast<int>(anchor.get_x());
    const auto delta_y = static_cast<int>(requested_tail.get_y()) - static_cast<int>(anchor.get_y());
    if (std::abs(delta_x) > std::abs(delta_y)) {
        return better::Position(requested_tail.get_x(), anchor.get_y());
    } else {
        return better::Position(anchor.get_x(), requested_tail.get_y());
    }
};

better::Position smallest_possible_tail(const better::Position& anchor) {
    if (anchor.get_x() > 0) {
        return better::Position{anchor.get_x() - 1, anchor.get_y()};
    } else {
        return better::Position{anchor.get_x() + 1, anchor.get_y()};
    }
};
