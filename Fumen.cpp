//
// Created by Syméon on 17/08/2017.
//

#include "Fumen.h"

bool Fumen::checkMemon(nlohmann::json j) {

	/* Prend en entrée un memon parsé avec nlohmann::json
	 * et vérifie :
	 * la présence de metadata avec : song_title (string)
	 *                                artist (string)
	 *                                music_path (string)
	 *                                jacket_path (string)
	 *                                BPM (number)
	 *                                offset (integer)
	 * la présence de data
	 * si il ya des charts il faut :
	 *      dif (string BSC, ADV, EXT, [2], ou autre)
	 *      level (le niveau en integer > 0)
	 *      resolution (un integer > 0)
	 *      et des notes
	 *
	 * pour chaque note il faut :
	 *      n (entre 0 et 15)
	 *      t (supérieur à 0)
	 *      l (supérieur à 0)
	 *      p (en accord avec n)
	 */

	bool metadata_correct = ((j.find("metadata") != j.end())
	                         and
	                         (j["metadata"].find("song_title") != j.end() and j["metadata"]["song_title"].is_string())
	                         and (j["metadata"].find("artist") != j.end() and j["metadata"]["artist"].is_string())
	                         and
	                         (j["metadata"].find("music_path") != j.end() and j["metadata"]["music_path"].is_string())
	                         and
	                         (j["metadata"].find("jacket_path") != j.end() and j["metadata"]["jacket_path"].is_string())
	                         and (j["metadata"].find("BPM") != j.end() and j["metadata"]["BPM"].is_number() and
	                              j["metadata"]["BPM"] > 0)
	                         and
	                         (j["metadata"].find("offset") != j.end() and j["metadata"]["offset"].is_number_integer()));

	bool data_correct = (j.find("data") != j.end());
	data_correct &= j["data"].is_array();

	//pour chaque chart
	for (auto &chart : j["data"]) {
		data_correct &= (chart.find("dif") != chart.end() and chart["dif"].is_string());
		data_correct &= (chart.find("level") != chart.end() and chart["level"].is_number_integer() and
		                 chart["level"] >= 0);
		data_correct &= (chart.find("resolution") != chart.end() and chart["resolution"].is_number_integer() and
		                 chart["resolution"] >= 0);
		data_correct &= (chart.find("notes") != chart.end() and chart["notes"].is_array());

		//pas besoin de check si les notes sont bonnes si y'a déjà un problème
		if (data_correct) {
			for (auto &note : chart["notes"]) {
				data_correct &= (note.find("n") != note.end() and note["n"].is_number_integer() and note["n"] >= 0 and
				                 note["n"] <= 15);
				data_correct &= (note.find("t") != note.end() and note["t"].is_number_integer() and note["t"] >= 0);
				data_correct &= (note.find("l") != note.end() and note["l"].is_number_integer() and note["l"] >= 0);
				data_correct &= (note.find("p") != note.end() and note["p"].is_number_integer() and note["p"]%12 == note["p"]);
				if (data_correct and note["l"] > 0) {
					data_correct &= Note::trail_pos_correct(note["n"],note["p"]);
				}
			}
		}
	}


	//le fumen est bon si les métadonnées et les données sont bonnes
	return metadata_correct and data_correct;
}

void Fumen::loadFromMemon(std::string path) {

	// for convenience
	using json = nlohmann::json;
	json j;
	std::ifstream fichier(path);

	fichier >> j;

	if (Fumen::checkMemon(j)) {
		this->setSongTitle(j["metadata"]["song_title"]);
		this->setArtist(j["metadata"]["artist"]);
		this->setMusicPath(j["metadata"]["music_path"]);
		this->setJacketPath(j["metadata"]["jacket_path"]);
		this->setBPM(j["metadata"]["BPM"]);
		this->setOffset(j["metadata"]["offset"]);
		// TODO : finir la désérialisation depuis un memon
	}
}

void Fumen::addNote(Note note) {
	this->Notes.insert(note);
}

void Fumen::removeNote(Note note) {
	if (hasNote(note)) {
		Notes.erase(Notes.find(note));
	}
}

bool Fumen::hasNote(Note note) {
	return this->Notes.find(note) != Notes.end();
}

const std::string &Fumen::getSongTitle() const {
	return songTitle;
}

void Fumen::setSongTitle(const std::string &songTitle) {
	Fumen::songTitle = songTitle;
}

const std::string &Fumen::getArtist() const {
	return artist;
}

void Fumen::setArtist(const std::string &artist) {
	Fumen::artist = artist;
}

const std::string &Fumen::getMusicPath() const {
	return musicPath;
}

void Fumen::setMusicPath(const std::string &musicPath) {
	Fumen::musicPath = musicPath;
}

const std::string &Fumen::getJacketPath() const {
	return jacketPath;
}

void Fumen::setJacketPath(const std::string &jacketPath) {
	Fumen::jacketPath = jacketPath;
}

float Fumen::getBPM() const {
	return BPM;
}

void Fumen::setBPM(float BPM) {
	Fumen::BPM = BPM;
}

float Fumen::getOffset() const {
	return offset;
}

void Fumen::setOffset(float offset) {
	Fumen::offset = offset;
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
                             offset(offset),
							 Notes({}) {}
