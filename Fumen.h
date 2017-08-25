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


class Fumen {

public:

	Fumen(const std::string &songTitle = "",
	      const std::string &artist = "",
	      const std::string &musicPath = "",
	      const std::string &jacketPath = "",
	      float BPM = 120,
	      float offset = 0);

	void loadFromMemon(std::string path);
	void loadFromMemo(std::string path);
	void loadFromEve(std::string path);

	void saveAsMemon(std::string path);
	void saveAsMemo(std::string path);
	void saveAsEve(std::string path);

	void addNote(Note note);
	void removeNote(Note note);
	bool hasNote(Note note);

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

	std::set<Note> Notes;
	std::string songTitle;
	std::string artist;
	std::string musicPath;
	std::string jacketPath;
	float BPM;
	float offset;
	bool checkMemon(nlohmann::json j);

};


#endif //FEIS_FUMEN_H
