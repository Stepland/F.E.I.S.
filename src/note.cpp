#include <stdexcept>
#include <assert.h>
#include <optional>
#include "note.hpp"

Note::Note(int pos, int timing, int length, int tail_pos) {
	if (timing<0) {
		throw std::runtime_error("Tried creating a note with negative timing : "+std::to_string(timing));
	}
	if (!(pos>=0 and pos<=15)) {
		throw std::runtime_error("Tried creating a note with invalid position : "+std::to_string(pos));
	}
	if (length<0) {
		throw std::runtime_error("Tried creating a note with invalid length : "+std::to_string(length));
	}
	if (length > 0) {
		if (tail_pos < 0 or tail_pos > 11 or !tail_pos_correct(pos, tail_pos)) {
			throw std::runtime_error(
					"Tried creating a long note with invalid tail position : " + std::to_string(tail_pos));
		}
	}
	this->timing = timing;
	this->pos = pos;
	this->length = length;
	this->tail_pos = tail_pos;

}

/*
 * Constructor to create a long note out of a pair
 */
Note::Note(const Note& note_a, const Note& note_b) {
	this->initAsClosestLongNote(note_a, note_b.timing, note_b.pos);
}

void Note::initAsClosestLongNote(const Note &start, int end_timing, int wanted_tail_pos) {
	pos = start.getPos();
	timing = std::min(start.getTiming(), end_timing);
	length = std::abs(start.getTiming() - end_timing);

	std::optional<int> best_tail_pos = {};
	for (int i = 0; i < 12; ++i) {
		if (Note::tail_pos_correct(pos,i)) {
			if (not best_tail_pos) {
				best_tail_pos = i;
			} else {
				int potential_tail = Note::tail_pos_to_note_pos(pos,i);
				int best_tail = Note::tail_pos_to_note_pos(pos, *best_tail_pos);
				if (distance(potential_tail, wanted_tail_pos) < distance(best_tail, wanted_tail_pos)) {
					best_tail_pos = i;
				}
			}
		}
	}

	assert(best_tail_pos.has_value());
	tail_pos = *best_tail_pos;
}

bool Note::tail_pos_correct(int n, int p) {

	assert(n >= 0 and n <= 15);
	assert(p >= 0 and p <= 11);

	int x = n%4;
	int y = n/4;

	int dx = 0;
	int dy = 0;

    // Vertical
	if (p%2 == 0) {

        // Going up
	    if ((p/2)%2 == 0) {
	        dy = -(p/4 + 1);

        // Going down
	    } else {
	        dy = p/4 +1;
	    }

    // Horizontal
	} else {

	    // Going right
	    if ((p/2)%2 == 0) {
	        dx = p/4 + 1;

        // Going left
	    } else {
	        dx = -(p/4 + 1);
	    }

	}

	return ((0 <= x+dx) and (x+dx <= 4)) and ((0 <= y+dy) and (y+dy <= 4));

}

int Note::tail_pos_to_note_pos(int pos, int tail_pos) {

	int x = pos%4;
	int y = pos/4;

	int dx = 0;
	int dy = 0;

	// Vertical
	if (tail_pos%2 == 0) {

		// Going up
		if ((tail_pos/2)%2 == 0) {
			dy = -(tail_pos/4 + 1);

			// Going down
		} else {
			dy = tail_pos/4 +1;
		}

		// Horizontal
	} else {

		// Going right
		if ((tail_pos/2)%2 == 0) {
			dx = tail_pos/4 + 1;

			// Going left
		} else {
			dx = -(tail_pos/4 + 1);
		}

	}

	return x+dx + 4*(y+dy);
}

int Note::distance(int pos_a, int pos_b) {
	int x = (pos_a%4) - (pos_b%4);
	int y = (pos_a/4) - (pos_b/4);
	return x*x + y*y;
}

int Note::getPos() const {
	return pos;
}

int Note::getLength() const {
	return length;
}

int Note::getTail_pos() const {
	return tail_pos;
}

int Note::getTiming() const {
	return timing;
}

bool Note::operator==(const Note &rhs) const {
	return timing == rhs.timing &&
	       pos == rhs.pos;
}

bool Note::operator!=(const Note &rhs) const {
	return !(rhs == *this);
}

bool Note::operator<(const Note &rhs) const {
	if (timing < rhs.timing)
		return true;
	if (rhs.timing < timing)
		return false;
	return pos < rhs.pos;
}

bool Note::operator>(const Note &rhs) const {
	return rhs < *this;
}

bool Note::operator<=(const Note &rhs) const {
	return !(rhs < *this);
}

bool Note::operator>=(const Note &rhs) const {
	return !(*this < rhs);
}
