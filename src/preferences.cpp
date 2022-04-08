#include "preferences.hpp"
#include <cstddef>
#include <exception>
#include "json.hpp"
#include "src/marker.hpp"

const std::string preference_file = "preferences.json";

Preferences::Preferences(std::filesystem::path assets, std::filesystem::path settings) :
    marker_ending_state(Judgement::Perfect),
    file_path(settings / preference_file)
{
    bool found_a_marker = false;
    const auto markers_folder = assets / "textures" / "markers";
    for (auto& folder : std::filesystem::directory_iterator(markers_folder)) {
        try {
            Marker m{folder.path()};
            marker = folder.path();
            found_a_marker = true;
        } catch (const std::exception&) {
            continue;
        }
    }
    if (not found_a_marker) {
        throw std::runtime_error("No valid marker found");
    }

    if (std::filesystem::exists(file_path)) {
        nlohmann::json j;
        std::ifstream preferences_file(file_path);
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

void Preferences::load_v0_1_0(nlohmann::json j) {
    auto new_marker_path = j.at("marker").at("folder").get<std::u8string>();
    try {
        Marker m{new_marker_path};
        marker = std::filesystem::path(new_marker_path);
    } catch (const std::exception&) {}

    auto new_marker_ending_state = j.at("marker").at("ending state").get<std::string>();
    for (const auto& [judement, name] : marker_state_previews) {
        if (new_marker_ending_state == name) {
            marker_ending_state = judement;
        }
    }
}

void Preferences::save() {
    std::ofstream preferences_file(file_path);

    nlohmann::ordered_json j = {{"version", "0.1.0"}, {"marker", nlohmann::json::object()}};

    j["marker"]["folder"] = marker.u8string();

    bool found = false;
    for (const auto& [judgement, name] : marker_state_previews) {
        if (marker_ending_state == judgement) {
            j["marker"]["ending state"] = name;
            found = true;
            break;
        }
    }
    if (not found) {
        throw std::runtime_error(
            "Could not find print name associated with marker ending state");
    }

    preferences_file << j.dump(4) << std::endl;
    preferences_file.close();
}
