//
// Created by Syméon on 25/08/2017.
//

#ifndef FEIS_CHART_H
#define FEIS_CHART_H

#include <iostream>
#include <set>
#include "Note.h"

// TODO : finir la classe Chart

class Chart {

public:

	Chart(const std::string &dif,
	      int level = 1,
	      int resolution = 240);

	const std::string &getDif() const;
	void setDif(const std::string &dif);

	int getLevel() const;
	void setLevel(int level);

	int getResolution() const;
	void setResolution(int resolution);

	void addNote(Note note);
	void removeNote(Note note);
	bool hasNote(Note note);
	int getNoteCount();
	const std::set<Note> &getNotes() const;

private:

	std::string dif;
	int level;
	int resolution;
	std::set<Note> Notes;

};


#endif //FEIS_CHART_H
