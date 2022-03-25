#pragma once

#include "better_note.hpp"
#include "better_notes.hpp"
#include "better_song.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "notes_clipboard.hpp"
#include "time_selection.hpp"
#include "widgets/density_graph.hpp"

#include <boost/concept_check.hpp>
#include <boost/multiprecision/detail/default_ops.hpp>
#include <filesystem>
#include <variant>
#include <utility>

using TapNotePair = std::pair<better::TapNote, better::TapNote>;

struct ChartState {
    ChartState(better::Chart& c, const std::string& name, std::filesystem::path assets);
    better::Chart& chart;
    const std::string& difficulty_name;
    better::Notes selected_notes;
    NotesClipboard notes_clipboard;
    SelectionState time_selection;

    /*
    The long note currently being created, represented as a pair of tap notes.
    Time span of the represented long note is the span between both taps
    1st note's position defines the long note's position
    2nd note's position suggests where the tail should start
    */
    std::optional<TapNotePair> long_note_being_created;

    // Is the user currently holding right click ? (over the playfield or not)
    bool creating_long_note;
    History<std::shared_ptr<ActionWithMessage>> history;
    DensityGraph density_graph;
};

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
