//
// Created by Syméon on 17/08/2017.
//
// Side note :
//
//  The tail position could be stored by a number between 0 and 5, if you look at it carefully
//  every note has exactly 6 different possible LN tail positions,
//  Doing this would allow any tuple of 4 numbers verifying :
//
//      0 <= position <= 15
//      0 <= timing
//      0 <= length
//      0 <= tail position <= 5
//
//  To count as a valid note, this would also allow for a purely json-schema based chart validation. since it
//  removes the need for a fancy consistency check between the note and its tail positions

#ifndef FEIS_NOTE_H
#define FEIS_NOTE_H

/*
 * A Note has :
 *
 * 	- a Position, from 0 to 15 given this way :
 *
 * 		 0  1  2  3
 * 		 4  5  6  7
 * 		 8  9 10 11
 * 		12 13 14 15
 *
 * 	- a Timing value, just a positive integer
 *
 * 	- a Length, set to 0 if not a long note, else a positive integer in the same unit as the note's timing
 *
 * 	- a Tail position, currently given this way :
 *
 *                8
 *                4
 *                0
 * 	    11  7  3  .  1  5  9
 * 	              2
 * 	              6
 * 	             10
 *
 * 	   with the . marking the note position
 * 	   gets ignored if the length is zero
 */
class Note {

public:


	Note(int pos, int timing, int length = 0, int tail_pos = 0);
	static bool tail_pos_correct(int pos, int tail_pos);

	bool operator==(const Note &rhs) const;
	bool operator!=(const Note &rhs) const;
	bool operator<(const Note &rhs) const;
	bool operator>(const Note &rhs) const;
	bool operator<=(const Note &rhs) const;
	bool operator>=(const Note &rhs) const;

	int getTiming() const;
	int getPos() const;
	int getLength() const;
	int getTail_pos() const;
	int getTail_pos_as_note_pos() const;

private:

	int timing;
	int pos;
	int length;
	int tail_pos;

};


#endif //FEIS_NOTE_H
