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

// TODO : trouver une manière ÉLÉGANTE d'acceder aux différentes charts

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

	void addChart(Chart chart);
	void removeChartByIndex(int index);
	void removeAllChartsWithName(std::string dif);
	bool hasChartWithName(std::string name);
	Chart& getChartByIndex(int index);
	Chart& getFirstChartWithName(std::string name);

	const std::string &getSongTitle() const;
	void setSongTitle(const std::string &songTitle);

	const std::string &getArtist() const;
	void setArtist(const std::string &artist);

	const std::string &getMusicPath() const;
	void setMusicPath(const std::string &musicPath);

	const std::string &getJacketPath() const;
	void setJacketPath(const std::string &jacketPath);

	float getBPM() const;
	void setBPM(float BPM);

	float getOffset() const;
	void setOffset(float offset);

private:

	std::vector<Chart> Charts;
	std::string songTitle;
	std::string artist;
	std::string musicPath;
	std::string jacketPath;
	float BPM;
	float offset;
	bool checkMemon(nlohmann::json j);

};


#endif //FEIS_FUMEN_H
