//
// Created by Syméon on 17/08/2017.
//

#include "Fumen.h"

void Fumen::loadFromMemon(std::string path) {

	// for convenience
	using json = nlohmann::json;
	json j;
	std::ifstream fichier(path);

	fichier >> j;

	this->songTitle = j.at("metadata").value("song title","");
	this->artist = j.at("metadata").value("artist","");
	this->musicPath = j.at("metadata").value("music path","");
	this->jacketPath = j.at("metadata").value("jacket path","");
	for (auto& chart_json : j.at("data")) {
		Chart chart = Chart(chart_json.at("dif_name"),chart_json.value("level",0),chart_json.at("resolution"));
		for (auto& note : chart_json.at("notes")) {
			chart.Notes.emplace(note.at("n"),note.at("t"),note.at("l"),note.at("p"));
		}
		this->Charts[chart.dif_name] = chart;
	}
}

void Fumen::saveAsMemon(std::string path) {

	std::ofstream fichier(path);
	using json = nlohmann::json;
	json j;
	j["metadata"] = json::object();
	j["metadata"]["song_title"] = this->songTitle;
	j["metadata"]["artist"] = this->artist;
	j["metadata"]["music_path"] = this->musicPath;
	j["metadata"]["jacket_path"] = this->jacketPath;
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

}

Fumen::Fumen(const std::string &songTitle,
             const std::string &artist,
             const std::string &musicPath,
             const std::string &jacketPath,
             float BPM,
             float offset) : songTitle(songTitle),
                             artist(artist),
                             musicPath(musicPath),
                             jacketPath(jacketPath),
                             BPM(BPM),
                             offset(offset) {}
