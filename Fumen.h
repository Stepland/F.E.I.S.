//
// Created by Syméon on 17/08/2017.
//

#ifndef FEIS_FUMEN_H
#define FEIS_FUMEN_H

#include <iostream>
#include <map>


class Fumen {
public:
	Fumen();
	void loadFromMemon(std::string path) {};
	void loadFromMemo(std::string path) {};
	void loadFromEve(std::string path) {};
	void setNote(int timing,int note,int length,int trail_pos) {};

private:
	std::multimap<int,Note> Notes;
};


#endif //FEIS_FUMEN_H
