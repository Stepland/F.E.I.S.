//
// Created by Syméon on 17/08/2017.
//

#ifndef FEIS_FUMEN_H
#define FEIS_FUMEN_H

#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <json/json.hpp>

#include "Note.h"
#include "Chart.h"

class Fumen {

public:

	Fumen(const std::string &songTitle = "",
	      const std::string &artist = "",
	      const std::string &musicPath = "",
	      const std::string &jacketPath = "",
	      float BPM = 120,
	      float offset = 0);

	void loadFromMemon(std::string path);
	// TODO : implementer ça
	//void loadFromMemo(std::string path);
	//void loadFromEve(std::string path);

	void saveAsMemon(std::string path);
	// TODO : implementer ça
	//void saveAsMemo(std::string path);
	//void saveAsEve(std::string path);

	std::map<std::string,Chart> Charts;
	std::string songTitle;
	std::string artist;
	std::string musicPath;
	std::string jacketPath;
	float BPM;
	float offset;

};


#endif //FEIS_FUMEN_H
