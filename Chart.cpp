//
// Created by Syméon on 25/08/2017.
//

#include "Chart.h"

void Chart::addNote(Note note) {
	this->Notes.insert(note);
}

bool Chart::hasNote(Note note) {
	return this->Notes.find(note) != Notes.end();
}

void Chart::removeNote(Note note) {
	if (hasNote(note)) {
		Notes.erase(Notes.find(note));
	}
}

const std::string &Chart::getDif() const {
	return dif;
}

void Chart::setDif(const std::string &dif) {
	Chart::dif = dif;
}

int Chart::getLevel() const {
	return level;
}

void Chart::setLevel(int level) {
	Chart::level = level;
}

int Chart::getResolution() const {
	return resolution;
}

void Chart::setResolution(int resolution) {
	Chart::resolution = resolution;
}

int Chart::getNoteCount() {
	return this->Notes.size();
}

Chart::Chart(const std::string &dif, int level, int resolution) : dif(dif),
                                                                  level(level),
                                                                  resolution(resolution),
                                                                  Notes() {}

const std::set<Note> &Chart::getNotes() const {
	return Notes;
}
