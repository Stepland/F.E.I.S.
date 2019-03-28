//
// Created by Syméon on 28/03/2019.
//

#include "EditActions.h"

void EditActions::undo(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart) {
        auto previous = ed->chart->history.get_previous();
        if (previous) {
            nq.push(std::make_shared<UndoNotification>(**previous));
            (*previous)->undoAction(*ed);
            ed->densityGraph.should_recompute = true;
        }
    }
}

void EditActions::redo(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart) {
        auto next = ed->chart->history.get_next();
        if (next) {
            nq.push(std::make_shared<RedoNotification>(**next));
            (*next)->doAction(*ed);
            ed->densityGraph.should_recompute = true;
        }
    }
}


void EditActions::cut(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart and (not ed->chart->selectedNotes.empty())) {

        std::stringstream ss;
        ss << "Cut " << ed->chart->selectedNotes.size() << " note";
        if (ed->chart->selectedNotes.size() > 1) {
            ss << "s";
        }
        nq.push(std::make_shared<TextNotification>(ss.str()));

        ed->chart->notesClipboard.copy(ed->chart->selectedNotes);
        for (auto note : ed->chart->selectedNotes) {
            ed->chart->ref.Notes.erase(note);
        }
        ed->chart->history.push(std::make_shared<ToggledNotes>(ed->chart->selectedNotes,false));
        ed->chart->selectedNotes.clear();
    }
}

void EditActions::copy(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart and (not ed->chart->selectedNotes.empty())) {

        std::stringstream ss;
        ss << "Copied " << ed->chart->selectedNotes.size() << " note";
        if (ed->chart->selectedNotes.size() > 1) {
            ss << "s";
        }
        nq.push(std::make_shared<TextNotification>(ss.str()));

        ed->chart->notesClipboard.copy(ed->chart->selectedNotes);
    }
}

void EditActions::paste(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart and (not ed->chart->notesClipboard.empty())) {

        int tick_offset = static_cast<int>(ed->getCurrentTick());
        std::set<Note> pasted_notes = ed->chart->notesClipboard.paste(tick_offset);

        std::stringstream ss;
        ss << "Pasted " << pasted_notes.size() << " note";
        if (pasted_notes.size() > 1) {
            ss << "s";
        }
        nq.push(std::make_shared<TextNotification>(ss.str()));

        for (auto note : pasted_notes) {
            ed->chart->ref.Notes.insert(note);
        }
        ed->chart->selectedNotes = pasted_notes;
        ed->chart->history.push(std::make_shared<ToggledNotes>(ed->chart->selectedNotes,true));
    }

}

void EditActions::delete_(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart) {
        if (not ed->chart->selectedNotes.empty()) {
            ed->chart->history.push(std::make_shared<ToggledNotes>(ed->chart->selectedNotes,false));
            nq.push(std::make_shared<TextNotification>("Deleted selected notes"));
            for (auto note : ed->chart->selectedNotes) {
                ed->chart->ref.Notes.erase(note);
            }
            ed->chart->selectedNotes.clear();
        }
    }
}