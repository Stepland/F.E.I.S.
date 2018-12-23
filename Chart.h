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

	int getResolution() const;
	void setResolution(int resolution);

	std::string dif_name;
	int level;
	std::set<Note> Notes;

private:

	int resolution;

};


#endif //FEIS_CHART_H
