//
// Created by Syméon on 17/08/2017.
//
//
//  Un objet marker contient les différentes textures d'un marker choisi
//

#ifndef FEIS_MARKER_H
#define FEIS_MARKER_H
#include <iostream>
#include <SFML/Graphics.hpp>
#include <map>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <functional>

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
    static std::vector<MarkerStatePreview> markerStatePreviews{
            {MarkerEndingState_PERFECT,"h402","PERFECT"},
            {MarkerEndingState_GREAT,"h302","GREAT"},
            {MarkerEndingState_GOOD,"h202","GOOD"},
            {MarkerEndingState_EARLY,"h102","EARLY / LATE"},
            {MarkerEndingState_MISS,"ma17","MISS"}
    };
}

/*
 * Holds the textures associated with a given marker folder from the assets folder
 */
class Marker {

public:

	Marker();
	explicit Marker(std::filesystem::path folder);
	std::optional<std::reference_wrapper<sf::Texture>> getSprite(MarkerEndingState state, float seconds);
    const std::map<std::string, sf::Texture> &getTextures() const;
    static bool validMarkerFolder(std::filesystem::path folder);

private:

	std::map<std::string,sf::Texture> textures;
	std::filesystem::path path;
	void initFromFolder(std::filesystem::path folder);

};


#endif //FEIS_MARKER_H
