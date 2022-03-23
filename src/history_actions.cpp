#include "history_actions.hpp"

#include <sstream>

#include "editor_state.hpp"

const std::string& ActionWithMessage::getMessage() const {
    return message;
}

OpenChart::OpenChart(Chart c) : notes(c.Notes) {
    std::stringstream ss;
    ss << "Opened Chart " << c.dif_name << " (Level " << c.level << ")";
    message = ss.str();
}

void OpenChart::doAction(EditorState& ed) const {
    ed.chart_state->chart.Notes = notes;
}

AddNotes::AddNotes(std::set<Note> n) : notes(n) {
    if (n.empty()) {
        throw std::invalid_argument(
            "Can't construct a AddedNotes History Action with an empty note "
            "set"
        );
    }
    std::stringstream ss;
    ss << "Added " << n.size() << " Note";
    if (n.size() > 1) {
        ss << "s";
    }
    message = ss.str();
}

void AddNotes::doAction(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->getTiming()));
    for (auto note : notes) {
        if (
            ed.chart_state->chart.Notes.find(note)
            == ed.chart_state->chart.Notes.end()
        ) {
            ed.chart_state->chart.Notes.insert(note);
        }
    }
}

void AddNotes::undoAction(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->getTiming()));
    for (auto note : notes) {
        if (
            ed.chart_state->chart.Notes.find(note)
            != ed.chart_state->chart.Notes.end()
        ) {
            ed.chart_state->chart.Notes.erase(note);
        }
    }
}


RemoveNotes::RemoveNotes(std::set<Note> n) : AddNotes(n) {
    if (n.empty()) {
        throw std::invalid_argument(
            "Can't construct a RemovedNotes History Action with an empty note "
            "set"
        );
    }

    std::stringstream ss;
    ss << "Removed " << n.size() << " Note";
    if (n.size() > 1) {
        ss << "s";
    }
    message = ss.str();
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
