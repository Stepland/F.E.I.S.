#ifndef FEIS_PREFERENCES_H
#define FEIS_PREFERENCES_H

#include <fstream>
#include <json.hpp>
#include <string>

#include "marker.hpp"

class Preferences {
public:
    Preferences();

    void load(nlohmann::json j);
    void load_v0_1_0(nlohmann::json j);
    void loadDefaults();

    void save();

    std::string marker;
    MarkerEndingState markerEndingState;

private:
    const std::string file_path = "settings/preferences.json";
};

#endif  // FEIS_PREFERENCES_H
