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

    std::filesystem::path marker;
    Judgement marker_ending_state;

private:
    std::filesystem::path file_path;
};
