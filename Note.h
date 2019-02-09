//
// Created by Syméon on 17/08/2017.
//
//
// Les notes ne contiennent pas en elles mêmes d'information sur leur timing, c'est la structure Fumen qui
// les contient qui s'en occupe
//
// Pour l'instant je vois pas trop la necessité de changer la position par contre la longeur et la position
// de la queue je pense que ça peut être utile
//

#ifndef FEIS_NOTE_H
#define FEIS_NOTE_H


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
