//
// Created by Syméon on 25/08/2017.
//

#include "Chart.h"

int Chart::getResolution() const {
	return resolution;
}

void Chart::setResolution(int resolution) {
	if (resolution <= 0) {
		throw std::invalid_argument("Can't set a resolution of "+std::to_string(resolution));
	} else {
		this->resolution = resolution;
	}
}

Chart::Chart(const std::string &dif, int level, int resolution) : dif_name(dif),
                                                                  level(level),
                                                                  resolution(resolution),
                                                                  Notes() {
	if (resolution <= 0) {
		throw std::invalid_argument("Can't set a resolution of "+std::to_string(resolution));
	}
}

bool Chart::operator==(const Chart &rhs) const {
    return dif_name == rhs.dif_name &&
           level == rhs.level &&
           Notes == rhs.Notes &&
           resolution == rhs.resolution;
}

bool Chart::operator!=(const Chart &rhs) const {
    return !(rhs == *this);
}

bool Chart::is_colliding(const Note &note, int ticks_threshold) {

	int lower_bound = note.getTiming()-ticks_threshold;
	int upper_bound = note.getTiming()+ticks_threshold;

	auto lower_note = Notes.lower_bound(Note(0,lower_bound));
	auto upper_note = Notes.upper_bound(Note(15,upper_bound));

	if (lower_note != Notes.end()) {
		for (auto other_note = lower_note; other_note != Notes.end() and other_note != upper_note; ++other_note) {
			if (other_note->getPos() == note.getPos() and other_note->getTiming() != note.getTiming()) {
				return true;
			}
		}
	}

	return false;
}
