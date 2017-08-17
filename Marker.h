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

enum Etat {
	APPROCHE,
	MISS,
	EARLY,
	GOOD,
	GREAT,
	PERFECT
};

class Marker {

public:

	Marker(std::string folder = "mk0013");
	sf::Sprite getSprite(Etat etat, int frame);

private:

	std::map<std::string,sf::Texture> textures;
	std::string path;

};


#endif //FEIS_MARKER_H
