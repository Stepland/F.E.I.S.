#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <json.hpp>
#include <map>
#include <set>

#include "chart.hpp"
#include "note.hpp"

/*
 * Difficulty name ordering : BSC > ADV > EXT > anything else in lexicographical
 * order
 */
struct cmpDifName {
    std::map<std::string, int> dif_names;

    cmpDifName() { dif_names = {{"BSC", 1}, {"ADV", 2}, {"EXT", 3}}; }
    bool operator()(const std::string& a, const std::string& b) const;
};

/*
 * Represents a .memon file : several charts and some metadata
 */
class Fumen {
public:
    explicit Fumen(
        const std::filesystem::path& path,
        const std::string& songTitle = "",
        const std::string& artist = "",
        const std::string& musicPath = "",
        const std::string& albumCoverPath = "",
        float BPM = 120,
        float offset = 0);

    void loadFromMemon(std::filesystem::path path);
    void loadFromMemon_v0_1_0(nlohmann::json j);
    void loadFromMemon_fallback(nlohmann::json j);

    void saveAsMemon(std::filesystem::path path);

    void autoLoadFromMemon() { loadFromMemon(path); };
    void autoSaveAsMemon() { saveAsMemon(path); };

    std::map<std::string, Chart, cmpDifName> Charts;
    std::filesystem::path path;
    std::string songTitle;
    std::string artist;
    std::string musicPath;
    std::string albumCoverPath;
    float BPM;
    float offset;

    float getChartRuntime(Chart c);
};
