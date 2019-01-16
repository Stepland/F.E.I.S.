//
// Created by Syméon on 25/08/2017.
//

#ifndef FEIS_CHART_H
#define FEIS_CHART_H

#include <iostream>
#include <set>
#include "Note.h"

class Chart {

public:

	Chart(const std::string &dif = "Edit",
	      int level = 1,
	      int resolution = 240);

	int getResolution() const;
	void setResolution(int resolution);

	std::string dif_name;
	int level;
	std::set<Note> Notes;

	bool operator==(const Chart &rhs) const;

	bool operator!=(const Chart &rhs) const;

private:

	int resolution;

};


#endif //FEIS_CHART_H
