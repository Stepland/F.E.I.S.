//
// Created by Syméon on 28/03/2019.
//

#include "EditorStateActions.h"

void Move::backwardsInTime(std::optional<EditorState> &ed) {
    if (ed and ed->chart) {
        float floatTicks = ed->getCurrentTick();
        auto prevTick = static_cast<int>(floorf(floatTicks));
        int step = ed->getSnapStep();
        int prevTickInSnap = prevTick;
        if (prevTick%step == 0) {
            prevTickInSnap -= step;
        } else {
            prevTickInSnap -= prevTick%step;
        }
        ed->setPlaybackAndMusicPosition(sf::seconds(ed->getSecondsAt(prevTickInSnap)));
    }
}

void Move::forwardsInTime(std::optional<EditorState> &ed) {
    if (ed and ed->chart) {
        float floatTicks = ed->getCurrentTick();
        auto nextTick = static_cast<int>(ceilf(floatTicks));
        int step = ed->getSnapStep();
        int nextTickInSnap = nextTick + (step - nextTick%step);
        ed->setPlaybackAndMusicPosition(sf::seconds(ed->getSecondsAt(nextTickInSnap)));
    }
}

void Edit::undo(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart) {
        auto previous = ed->chart->history.get_previous();
        if (previous) {
            nq.push(std::make_shared<UndoNotification>(**previous));
            (*previous)->undoAction(*ed);
            ed->chart->densityGraph.should_recompute = true;
        }
    }
}

void Edit::redo(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart) {
        auto next = ed->chart->history.get_next();
        if (next) {
            nq.push(std::make_shared<RedoNotification>(**next));
            (*next)->doAction(*ed);
            ed->chart->densityGraph.should_recompute = true;
        }
    }
}


void Edit::cut(std::optional<EditorState>& ed, NotificationsQueue& nq) {
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

void Edit::copy(std::optional<EditorState>& ed, NotificationsQueue& nq) {
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

void Edit::paste(std::optional<EditorState>& ed, NotificationsQueue& nq) {
    if (ed and ed->chart and (not ed->chart->notesClipboard.empty())) {

        auto tick_offset = static_cast<int>(ed->getCurrentTick());
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

void Edit::delete_(std::optional<EditorState>& ed, NotificationsQueue& nq) {
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