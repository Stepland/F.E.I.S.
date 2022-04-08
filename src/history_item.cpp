#include "history_item.hpp"

#include <functional>
#include <sstream>
#include <tuple>

#include <fmt/core.h>

#include "better_song.hpp"
#include "editor_state.hpp"


const std::string& HistoryItem::get_message() const {
    return message;
}

OpenChart::OpenChart(const better::Chart& c, const std::string& difficulty) : notes(c.notes) {
    if (c.level) {
        message = fmt::format(
            "Opened Chart {} (level {})",
            difficulty,
            c.level->format("f")
        );
    } else {
        message = fmt::format(
            "Opened Chart {} (no level defined)",
            difficulty
        );
    }
}

void OpenChart::do_action(EditorState& ed) const {
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

void AddNotes::do_action(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->second.get_time()));
    if (ed.chart_state) {
        for (const auto& [_, note] : notes) {
            ed.chart_state->chart.notes.insert(note);
        }
    }
}

void AddNotes::undo_action(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->second.get_time()));
    if (ed.chart_state) {
        for (const auto& [_, note] : notes) {
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

void RemoveNotes::do_action(EditorState& ed) const {
    AddNotes::undo_action(ed);
}

void RemoveNotes::undo_action(EditorState& ed) const {
    AddNotes::do_action(ed);
}

std::string get_message(const std::shared_ptr<HistoryItem>& awm) {
    return awm->get_message();
}
