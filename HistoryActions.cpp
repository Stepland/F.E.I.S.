//
// Created by Syméon on 02/03/2019.
//

#include <sstream>
#include "HistoryActions.h"
#include "EditorState.h"

const std::string &ActionWithMessage::getMessage() const {
    return message;
}

OpenChart::OpenChart(Chart c) : notes(c.Notes) {
    std::stringstream ss;
    ss << "Opened Chart " << c.dif_name << " (Level " << c.level << ")";
    message = ss.str();
}

void OpenChart::doAction(EditorState &ed) {
    ed.chart->ref.Notes = notes;
}

ToggledNotes::ToggledNotes(std::set<Note> n, bool have_been_added) : notes(n), have_been_added(have_been_added) {
    std::stringstream ss;
    if (have_been_added) {
        ss << "Added Note";
    } else {
        ss << "Removed Note";
    }
    if (n.size() > 1) {
        ss << "s";
    }
    message = ss.str();
}

void ToggledNotes::doAction(EditorState &ed) {
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

void ToggledNotes::undoAction(EditorState &ed) {
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
