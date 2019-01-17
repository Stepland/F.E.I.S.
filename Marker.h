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

class Marker {

public:

	Marker(std::string folder = "mk0013");
	std::optional<std::reference_wrapper<sf::Texture>> getSprite(MarkerEndingState state, float seconds);

    const std::map<std::string, sf::Texture> &getTextures() const;

private:

	std::map<std::string,sf::Texture> textures;
	std::string path;

};


#endif //FEIS_MARKER_H
