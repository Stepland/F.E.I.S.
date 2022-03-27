#include "history_actions.hpp"

#include <functional>
#include <sstream>
#include <tuple>

#include <fmt/core.h>

#include "better_song.hpp"
#include "editor_state.hpp"
#include "std_optional_extras.hpp"


const std::string& ActionWithMessage::getMessage() const {
    return message;
}

OpenChart::OpenChart(better::Chart c, const std::string& difficulty) : notes(c.notes) {
    message = fmt::format(
        "Opened Chart {} (level {})",
        difficulty,
        better::stringify_level(c.level)
    );
}

void OpenChart::doAction(EditorState& ed) const {
    if (ed.chart_state) {
        ed.chart_state->chart.notes = notes;
    }
}

AddNotes::AddNotes(const better::Notes& notes) : notes(notes) {
    if (notes.empty()) {
        throw std::invalid_argument(
            "Can't construct a AddedNotes History Action with an empty note "
            "set"
        );
    }
    message = fmt::format(
        "Added {} Note{}",
        notes.size(),
        notes.size() > 1 ? "s" : ""
    );
}

void AddNotes::doAction(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->second.get_time()));
    if (ed.chart_state) {
        for (auto note : notes) {
            ed.chart_state->chart.notes.insert(note);
        }
    }
}

void AddNotes::undoAction(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->second.get_time()));
    if (ed.chart_state) {
        for (auto note : notes) {
            ed.chart_state->chart.notes.erase(note);
        }
    }
}


RemoveNotes::RemoveNotes(const better::Notes& notes) : AddNotes(notes) {
    if (notes.empty()) {
        throw std::invalid_argument(
            "Can't construct a RemovedNotes History Action with an empty note "
            "set"
        );
    }
    message = fmt::format(
        "Removed {} Note{}",
        notes.size(),
        notes.size() > 1 ? "s" : ""
    );
}

void RemoveNotes::doAction(EditorState& ed) const {
    AddNotes::undoAction(ed);
}

void RemoveNotes::undoAction(EditorState& ed) const {
    AddNotes::doAction(ed);
}

std::string get_message(const std::shared_ptr<ActionWithMessage>& awm) {
    return awm->getMessage();
}
