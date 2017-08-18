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

	Note(int pos,int length,int trail_pos);
	static bool trail_pos_correct(int pos,int trail_pos);

	int getPos() const;
	int getLength() const;
	int getTrail_pos() const;
	void setLength(int length);
	void setTrail_pos(int trail_pos);

private:

	int pos;
	int length;
	int trail_pos;

};


#endif //FEIS_NOTE_H
