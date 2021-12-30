//
// Created by Syméon on 28/03/2019.
//

#ifndef FEIS_CHARTWITHHIST_H
#define FEIS_CHARTWITHHIST_H

#include "Chart.h"
#include "NotesClipboard.h"
#include "TimeSelection.h"
#include "HistoryActions.h"
#include "History.h"
#include "Widgets/DensityGraph.h"


struct Chart_with_History {
    explicit Chart_with_History(Chart &c);
    Chart& ref;
    std::set<Note> selectedNotes;
    NotesClipboard notesClipboard;
    SelectionState timeSelection;
    std::optional<std::pair<Note,Note>> longNoteBeingCreated;
    bool creatingLongNote;
    History<std::shared_ptr<ActionWithMessage>> history;
    DensityGraph densityGraph;

    std::optional<Note> makeLongNoteDummy(int current_tick) const;
    std::optional<Note> makeCurrentLongNote() const;
};

#endif //FEIS_CHARTWITHHIST_H
