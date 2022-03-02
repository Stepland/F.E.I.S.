#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

enum MarkerEndingState {
    MarkerEndingState_MISS,
    MarkerEndingState_EARLY,
    MarkerEndingState_GOOD,
    MarkerEndingState_GREAT,
    MarkerEndingState_PERFECT
};

struct MarkerStatePreview {
    MarkerEndingState state;
    std::string textureName;
    std::string printName;
};

namespace Markers {
    static std::vector<MarkerStatePreview> markerStatePreviews {
        {MarkerEndingState_PERFECT, "h402", "PERFECT"},
        {MarkerEndingState_GREAT, "h302", "GREAT"},
        {MarkerEndingState_GOOD, "h202", "GOOD"},
        {MarkerEndingState_EARLY, "h102", "EARLY / LATE"},
        {MarkerEndingState_MISS, "ma17", "MISS"}};
}

/*
 * Holds the textures associated with a given marker folder from the assets
 * folder
 */
class Marker {
public:
    explicit Marker(std::filesystem::path folder);
    std::optional<std::reference_wrapper<sf::Texture>>
    getSprite(MarkerEndingState state, float seconds);
    const std::map<std::string, sf::Texture>& getTextures() const;
    static bool validMarkerFolder(std::filesystem::path folder);

private:
    std::map<std::string, sf::Texture> textures;
    std::filesystem::path path;
    void initFromFolder(std::filesystem::path folder);
};

Marker first_available_marker_from_folder(std::filesystem::path assets_folder);
