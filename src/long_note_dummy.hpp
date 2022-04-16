#pragma once

#include <utility>

#include "better_note.hpp"
#include "special_numeric_types.hpp"

using TapNotePair = std::pair<better::TapNote, better::TapNote>;

/*
Construct a note to be displayed on the playfield as a preview of the long note
currently being created. It's basically the same at the real long note being
created but its start time is set to exactly the (given) current beat so the
long note drawing routine of the playfield can be repurposed as-is for the
preview
*/
better::LongNote make_long_note_dummy(
    Fraction current_beat,
    const TapNotePair& long_note_being_created
);

// Turn the tap pair into the real long note the user wants to create
better::LongNote make_long_note(const TapNotePair& long_note_being_created);

better::Position closest_tail_position(
    const better::Position& anchor,
    const better::Position& requested_tail
);

better::Position smallest_possible_tail(const better::Position& anchor);