#include "chart_state.hpp"

ChartState::ChartState(better::Chart& c, std::filesystem::path assets) :
    chart(c),
    density_graph(assets)
{
    history.push(std::make_shared<OpenChart>(c));
}

std::optional<Note> ChartState::makeLongNoteDummy(int current_tick) const {
    if (creating_long_note and long_note_being_created) {
        Note long_note = Note(long_note_being_created->first, long_note_being_created->second);
        Note dummy_long_note = Note(
            long_note.getPos(),
            current_tick,
            chart.getResolution(),
            long_note.getTail_pos());
        return dummy_long_note;
    } else {
        return {};
    }
}

std::optional<Note> ChartState::makeCurrentLongNote() const {
    if (creating_long_note and long_note_being_created) {
        return Note(long_note_being_created->first, long_note_being_created->second);
    } else {
        return {};
    }
}
