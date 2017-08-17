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
	if (length=0)

}
