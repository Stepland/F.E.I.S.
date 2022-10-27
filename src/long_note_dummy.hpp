#pragma once

#include <utility>

#include "better_note.hpp"
#include "special_numeric_types.hpp"

using TapNotePair = std::pair<better::TapNote, better::TapNote>;

/*
Construct a special note that will only be used to repurpose the long note
drawing routine of the playfield so that it also displays the *preview* of the
long note currently being created. It's basically the same at the real long
note being created but its start time is set to the exactly current beat
(passed as a parameter).
*/
better::LongNote make_long_note_dummy_for_playfield(
    const Fraction& current_beat,
    const TapNotePair& long_note_being_created,
    const Fraction& snap
);

// Turn the tap pair into the real long note the user wants to create, this
// is also what's displayed on the linear view
better::LongNote make_long_note_dummy_for_linear_view(
    const TapNotePair& long_note_being_created,
    const Fraction& snap
);

better::Position closest_tail_position(
    const better::Position& anchor,
    const better::Position& requested_tail
);

better::Position smallest_possible_tail(const better::Position& anchor);