//
// Created by Syméon on 28/03/2019.
//

#include "Preferences.h"

Preferences::Preferences() : markerEndingState(MarkerEndingState_PERFECT) {

    loadDefaults();

    std::filesystem::path preferences_path(file_path);

    if (std::filesystem::exists(preferences_path)) {
        nlohmann::json j;
        std::ifstream preferences_file(preferences_path);
        preferences_file >> j;
        load(j);
    }
}

void Preferences::load(nlohmann::json j) {

    if (j.find("version") != j.end() and j.at("version").is_string()) {
        auto version = j.at("version").get<std::string>();
        if (version == "0.1.0") {
            load_v0_1_0(j);
        }
    }

}

void Preferences::loadDefaults() {

    bool found_a_marker;
    for (auto& folder : std::filesystem::directory_iterator("assets/textures/markers/")) {
        if (Marker::validMarkerFolder(folder.path())) {
            assert(folder.is_directory());
            marker = folder.path().string();
            found_a_marker = true;
            break;
        }
    }
    if (not found_a_marker) {
        throw std::runtime_error("No valid marker found");
    }

    markerEndingState = MarkerEndingState_PERFECT;
}

void Preferences::load_v0_1_0(nlohmann::json j) {

    auto new_marker_path = j.at("marker").at("folder").get<std::string>();
    if (Marker::validMarkerFolder(new_marker_path)) {
        marker = new_marker_path;
    }

    auto new_markerEndingState = j.at("marker").at("ending state").get<std::string>();
    for (const auto& state : Markers::markerStatePreviews) {
        if (new_markerEndingState == state.printName) {
            markerEndingState = state.state;
        }
    }

}

void Preferences::save() {

    std::ofstream preferences_file(file_path);

    nlohmann::json j = {
            {"version", "0.1.0"},
            {"marker", nlohmann::json::object()}
    };

    j["marker"]["folder"] = marker;

    bool found = false;
    for (const auto& state : Markers::markerStatePreviews) {
        if (markerEndingState == state.state) {
            j["marker"]["ending state"] = state.printName;
            found = true;
            break;
        }
    }
    if (not found) {
        throw std::runtime_error("Could not find print name associated with marker ending state");
    }

    preferences_file << j.dump(4) << std::endl;
    preferences_file.close();
}
