//
// Created by Syméon on 17/08/2017.
//

#include "Fumen.h"

bool cmpDifName::operator()(const std::string &a, const std::string &b) const {
	if (dif_names.find(a) != dif_names.end()) {
		if (dif_names.find(b) != dif_names.end()) {
			return dif_names.find(a)->second < dif_names.find(b)->second;
		} else {
			return true;
		}
	} else {
		if (dif_names.find(b) != dif_names.end()) {
			return false;
		} else {
			return a < b;
		}
	}
}

void Fumen::loadFromMemon(std::filesystem::path path) {

	// for convenience
	using json = nlohmann::json;
	json j;
	std::ifstream fichier(path);

	fichier >> j;

	this->songTitle = j.at("metadata").value("song title","");
	this->artist = j.at("metadata").value("artist","");
	this->musicPath = j.at("metadata").value("music path","");
	this->jacketPath = j.at("metadata").value("jacket path","");
	this->BPM = j.at("metadata").value("BPM",120.f);
	this->offset = j.at("metadata").value("offset",0.f);
	for (auto& chart_json : j.at("data")) {
		Chart chart(chart_json.at("dif_name"),chart_json.value("level",0),chart_json.at("resolution"));
		for (auto& note : chart_json.at("notes")) {
			chart.Notes.emplace(note.at("n"),note.at("t"),note.at("l"),note.at("p"));
		}
		this->Charts.insert(std::pair<std::string,Chart>(chart.dif_name,chart));
	}
}

void Fumen::saveAsMemon(std::filesystem::path path) {

	std::ofstream fichier(path);
	using json = nlohmann::json;
	json j;
	j["metadata"] = json::object();
	j["metadata"]["song title"] = this->songTitle;
	j["metadata"]["artist"] = this->artist;
	j["metadata"]["music path"] = this->musicPath;
	j["metadata"]["jacket path"] = this->jacketPath;
	j["metadata"]["BPM"] = this->BPM;
	j["metadata"]["offset"] = this->offset;
	j["data"] = json::array();
	for (auto& tuple : this->Charts) {
		json chart_json;
		chart_json["dif_name"] = tuple.second.dif_name;
		chart_json["level"] = tuple.second.level;
		chart_json["resolution"] = tuple.second.getResolution();
		chart_json["notes"] = json::array();
		for (auto& note : tuple.second.Notes) {
			json note_json;
			note_json["n"] = note.getPos();
			note_json["t"] = note.getTiming();
			note_json["l"] = note.getLength();
			note_json["p"] = note.getTrail_pos();
			chart_json["notes"].push_back(note_json);
		}
		j["data"].push_back(chart_json);
	}

	fichier << j.dump(4) << std::endl;
	fichier.close();

}

Fumen::Fumen(const std::filesystem::path &path,
			 const std::string &songTitle,
             const std::string &artist,
             const std::string &musicPath,
             const std::string &jacketPath,
             float BPM,
             float offset) : path(path),
             				 songTitle(songTitle),
                             artist(artist),
                             musicPath(musicPath),
                             jacketPath(jacketPath),
                             BPM(BPM),
                             offset(offset) {}

/*
 * Returns, in seconds as a float, how long the chart lasts according to the notes themselves
 */
float Fumen::getChartRuntime(Chart c) {
	if (!c.Notes.empty()) {
		Note last_note = *c.Notes.rbegin();
		return ((static_cast<float>(last_note.getTiming())/c.getResolution())/this->BPM)*60.f;
	} else {
		return 0;
	}
}
