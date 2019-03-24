//
// Created by Syméon on 17/08/2017.
//

#ifndef FEIS_FUMEN_H
#define FEIS_FUMEN_H

#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <json.hpp>

#include "Note.h"
#include "Chart.h"

struct cmpDifName {
	std::map<std::string,int> dif_names;

	cmpDifName() {
		dif_names = {{"BSC",1},{"ADV",2},{"EXT",3}};
	}
	bool operator()(const std::string& a, const std::string& b) const;
};

class Fumen {

public:

	explicit Fumen(
			const std::filesystem::path &path,
          	const std::string &songTitle = "",
          	const std::string &artist = "",
	      	const std::string &musicPath = "",
	      	const std::string &albumCoverPath = "",
	      	float BPM = 120,
	      	float offset = 0
	      			);

	void loadFromMemon(std::filesystem::path path);
	void loadFromMemon_v0_1_0(nlohmann::json j);
	void loadFromMemon_fallback(nlohmann::json j);
	// TODO : implementer ça
	//void loadFromMemo(std::string path);
	//void loadFromEve(std::string path);

	void saveAsMemon(std::filesystem::path path);
	// TODO : implementer ça
	//void saveAsMemo(std::string path);
	//void saveAsEve(std::string path);

	void autoLoadFromMemon() {loadFromMemon(path);};
	void autoSaveAsMemon() {saveAsMemon(path);};

	std::map<std::string,Chart,cmpDifName> Charts;
	std::filesystem::path path;
	std::string songTitle;
	std::string artist;
	std::string musicPath;
	std::string albumCoverPath;
	float BPM;
	float offset;

	float getChartRuntime(Chart c);

};


#endif //FEIS_FUMEN_H
