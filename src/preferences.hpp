#pragma once

#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <string>

#include "marker.hpp"

class Preferences {
public:
    Preferences(std::filesystem::path assets, std::filesystem::path settings);

    void load(nlohmann::json j);
    void load_v0_1_0(nlohmann::json j);

    void save();

    std::string marker;
    MarkerEndingState markerEndingState;
    const std::filesystem::path file_path;
};
