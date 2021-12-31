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
    ed.chart->ref.Notes = notes;
}

ToggledNotes::ToggledNotes(std::set<Note> n, bool have_been_added) :
    notes(n),
    have_been_added(have_been_added) {
    if (n.empty()) {
        throw std::invalid_argument(
            "Can't construct a ToogledNotes History Action with an empty note "
            "set");
    }

    std::stringstream ss;
    if (have_been_added) {
        ss << "Added " << n.size() << " Note";
    } else {
        ss << "Removed " << n.size() << " Note";
    }
    if (n.size() > 1) {
        ss << "s";
    }
    message = ss.str();
}

void ToggledNotes::doAction(EditorState& ed) const {
    ed.setPlaybackAndMusicPosition(sf::seconds(ed.getSecondsAt(notes.begin()->getTiming())));
    if (have_been_added) {
        for (auto note : notes) {
            if (ed.chart->ref.Notes.find(note) == ed.chart->ref.Notes.end()) {
                ed.chart->ref.Notes.insert(note);
            }
        }
    } else {
        for (auto note : notes) {
            if (ed.chart->ref.Notes.find(note) != ed.chart->ref.Notes.end()) {
                ed.chart->ref.Notes.erase(note);
            }
        }
    }
}

void ToggledNotes::undoAction(EditorState& ed) const {
    ed.setPlaybackAndMusicPosition(sf::seconds(ed.getSecondsAt(notes.begin()->getTiming())));
    if (not have_been_added) {
        for (auto note : notes) {
            if (ed.chart->ref.Notes.find(note) == ed.chart->ref.Notes.end()) {
                ed.chart->ref.Notes.insert(note);
            }
        }
    } else {
        for (auto note : notes) {
            if (ed.chart->ref.Notes.find(note) != ed.chart->ref.Notes.end()) {
                ed.chart->ref.Notes.erase(note);
            }
        }
    }
}

std::string get_message(const std::shared_ptr<ActionWithMessage>& awm) {
    return awm->getMessage();
}
