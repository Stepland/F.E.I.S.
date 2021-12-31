#include "fumen.hpp"

Fumen::Fumen(
    const std::filesystem::path& path,
    const std::string& songTitle,
    const std::string& artist,
    const std::string& musicPath,
    const std::string& albumCoverPath,
    float BPM,
    float offset) :
    path(path),
    songTitle(songTitle),
    artist(artist),
    musicPath(musicPath),
    albumCoverPath(albumCoverPath),
    BPM(BPM),
    offset(offset) {}

bool cmpDifName::operator()(const std::string& a, const std::string& b) const {
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

/*
 * Selects a version-specific parsing function according to what's indicated in
 * the file
 */
void Fumen::loadFromMemon(std::filesystem::path path) {
    nlohmann::json j;
    std::ifstream fichier(path);

    fichier >> j;

    if (j.find("version") != j.end() and j.at("version").is_string()) {
        if (j.at("version").get<std::string>() == "0.1.0") {
            this->loadFromMemon_v0_1_0(j);
        } else {
            this->loadFromMemon_fallback(j);
        }
    } else {
        this->loadFromMemon_fallback(j);
    }
}

/*
 * Memon schema v0.1.0 :
 * 	- "data" is an object mapping a difficulty name to a chart, this way we
 * get the dif. name uniqueness for free
 * 	- "jacket path" is now "album cover path" because why tf not
 */
void Fumen::loadFromMemon_v0_1_0(nlohmann::json memon) {
    this->songTitle = memon.at("metadata").value("song title", "");
    this->artist = memon.at("metadata").value("artist", "");
    this->musicPath = memon.at("metadata").value("music path", "");
    this->albumCoverPath = memon.at("metadata").value("album cover path", "");
    this->BPM = memon.at("metadata").value("BPM", 120.f);
    this->offset = memon.at("metadata").value("offset", 0.f);
    for (auto& [dif_name, chart_json] : memon.at("data").items()) {
        Chart chart(dif_name, chart_json.value("level", 0), chart_json.at("resolution"));
        for (auto& note : chart_json.at("notes")) {
            chart.Notes.emplace(note.at("n"), note.at("t"), note.at("l"), note.at("p"));
        }
        this->Charts.insert(std::pair<std::string, Chart>(chart.dif_name, chart));
    }
}

/*
 * Fallback memon parser
 * Respects the old schema, with notable quirks :
 *   - "data" is an array of charts
 *   - the album cover path field is named "jacket path"
 *   	("jaquette" made sense in French but is a bit far-fetched in English
 * unfortunately)
 */
void Fumen::loadFromMemon_fallback(nlohmann::json j) {
    this->songTitle = j.at("metadata").value("song title", "");
    this->artist = j.at("metadata").value("artist", "");
    this->musicPath = j.at("metadata").value("music path", "");
    this->albumCoverPath = j.at("metadata").value("jacket path", "");
    this->BPM = j.at("metadata").value("BPM", 120.f);
    this->offset = j.at("metadata").value("offset", 0.f);
    for (auto& chart_json : j.at("data")) {
        Chart chart(
            chart_json.at("dif_name"),
            chart_json.value("level", 0),
            chart_json.at("resolution"));
        for (auto& note : chart_json.at("notes")) {
            chart.Notes.emplace(note.at("n"), note.at("t"), note.at("l"), note.at("p"));
        }
        this->Charts.insert(std::pair<std::string, Chart>(chart.dif_name, chart));
    }
}

void Fumen::saveAsMemon(std::filesystem::path path) {
    std::ofstream fichier(path);
    using json = nlohmann::json;
    json j = {
        {"version", "0.1.0"},
        {"metadata",
         {{"song title", this->songTitle},
          {"artist", this->artist},
          {"music path", this->musicPath},
          {"album cover path", this->albumCoverPath},
          {"BPM", this->BPM},
          {"offset", this->offset}}},
        {"data", json::object()}};
    for (auto& tuple : this->Charts) {
        json chart_json = {
            {"level", tuple.second.level},
            {"resolution", tuple.second.getResolution()},
            {"notes", json::array()}};
        for (auto& note : tuple.second.Notes) {
            json note_json = {
                {"n", note.getPos()},
                {"t", note.getTiming()},
                {"l", note.getLength()},
                {"p", note.getTail_pos()}};
            chart_json["notes"].push_back(note_json);
        }
        j["data"].emplace(tuple.second.dif_name, chart_json);
    }

    fichier << j.dump(4) << std::endl;
    fichier.close();
}

/*
 * Returns how long the chart is in seconds as a float, from beat 0 to the last
 * note
 */
float Fumen::getChartRuntime(Chart c) {
    if (!c.Notes.empty()) {
        Note last_note = *c.Notes.rbegin();
        return ((static_cast<float>(last_note.getTiming()) / c.getResolution()) / this->BPM)
            * 60.f;
    } else {
        return 0;
    }
}
