//
// Created by Syméon on 17/08/2017.
//

#include <stdexcept>
#include "Note.h"

Note::Note(int pos, int length, int trail_pos) : {
	if (!(pos>=0 and pos<=15)) {
		throw std::runtime_error("Tried creating a note with invalid position : "+std::to_string(pos));
	}
	if (length<0) {
		throw std::runtime_error("Tried creating a note with invalid length : "+std::to_string(length));
	}
	if (length>0 and !trail_pos_correct(pos,trail_pos)) {
		throw std::runtime_error("Tried creating a long note with invalid trail position : "+std::to_string(trail_pos));
	}
	this->pos = pos;
	this->length = length;
	this->trail_pos = trail_pos;

}

bool Note::trail_pos_correct(int note, int trail_pos) {

	int dist;
	// où pointe la queue de la note longue ?
	switch(trail_pos%4) {

		//vers le haut ?
		case 0:
			dist = (note/4) - (trail_pos/4 + 1);
			break;

		//vers la droite ?
		case 1:
			dist = (note%4) + (trail_pos/4 + 1);
			break;

		//vers le bas ?
		case 2:
			dist = (note/4) + (trail_pos/4 + 1);
			break;

		//vers la gauche ?
		case 3:
			dist = (note%4) - (trail_pos/4 + 1);
			break;

		//wtf ? comment veux-tu qu'un modulo 4 fasse autre chose ?
		default:
			throw std::runtime_error("Unexpected modulo result when checking note trail position");
	}

	//on reste bien dans l'écran si la position de la queue est entre 0 et 3 inclus
	return (dist >= 0 and dist <= 3);

}

int Note::getPos() const {
	return pos;
}

int Note::getLength() const {
	return length;
}

int Note::getTrail_pos() const {
	return trail_pos;
}

void Note::setLength(int length) {
	Note::length = length;
}

void Note::setTrail_pos(int trail_pos) {
	Note::trail_pos = trail_pos;
}
