#pragma once

#include "better_song.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "notes_clipboard.hpp"
#include "time_selection.hpp"
#include "widgets/density_graph.hpp"

#include <filesystem>

struct ChartState {
    explicit ChartState(better::Chart& c, std::filesystem::path assets);
    better::Chart& chart;
    std::set<Note> selected_notes;
    NotesClipboard notes_clipboard;
    SelectionState time_selection;
    std::optional<std::pair<Note, Note>> long_note_being_created;
    bool creating_long_note;
    History<std::shared_ptr<ActionWithMessage>> history;
    DensityGraph density_graph;

    std::optional<Note> makeLongNoteDummy(int current_tick) const;
    std::optional<Note> makeCurrentLongNote() const;
};
