#ifndef FEIS_CHARTWITHHIST_H
#define FEIS_CHARTWITHHIST_H

#include "chart.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "notes_clipboard.hpp"
#include "time_selection.hpp"
#include "widgets/density_graph.hpp"

struct Chart_with_History {
    explicit Chart_with_History(Chart& c);
    Chart& ref;
    std::set<Note> selectedNotes;
    NotesClipboard notesClipboard;
    SelectionState timeSelection;
    std::optional<std::pair<Note, Note>> longNoteBeingCreated;
    bool creatingLongNote;
    History<std::shared_ptr<ActionWithMessage>> history;
    DensityGraph densityGraph;

    std::optional<Note> makeLongNoteDummy(int current_tick) const;
    std::optional<Note> makeCurrentLongNote() const;
};

#endif  // FEIS_CHARTWITHHIST_H
