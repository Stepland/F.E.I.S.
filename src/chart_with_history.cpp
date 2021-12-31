#include "chart_with_history.hpp"

Chart_with_History::Chart_with_History(Chart& c) : ref(c) {
    history.push(std::make_shared<OpenChart>(c));
}

std::optional<Note> Chart_with_History::makeLongNoteDummy(int current_tick) const {
    if (creatingLongNote and longNoteBeingCreated) {
        Note long_note = Note(longNoteBeingCreated->first, longNoteBeingCreated->second);
        Note dummy_long_note = Note(
            long_note.getPos(),
            current_tick,
            ref.getResolution(),
            long_note.getTail_pos());
        return dummy_long_note;
    } else {
        return {};
    }
}

std::optional<Note> Chart_with_History::makeCurrentLongNote() const {
    if (creatingLongNote and longNoteBeingCreated) {
        return Note(longNoteBeingCreated->first, longNoteBeingCreated->second);
    } else {
        return {};
    }
}
