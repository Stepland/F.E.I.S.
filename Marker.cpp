//
// Created by Syméon on 17/08/2017.
//

#include "Marker.h"

Marker::Marker(std::string folder) {

	path = "assets/textures/markers/" + folder;

	// Chargement des h100~115 / h200~215 / h300~h315 / h400~415
	for (int sup = 1; sup<=4; sup++) {
		for (int i = 0; i <= 15; i++) {
			sf::Texture tex;
			if (!tex.loadFromFile(path + "/h" + std::to_string(i+100*sup) + ".png")) {
				std::cerr << "Unable to load marker " << folder;
				throw std::runtime_error("Unable to load marker " + folder);
			}
			textures.insert({"h" + std::to_string(i+100*sup), tex});
		}
	}

	// Chargement de ma00~23
	for (int i = 0; i <= 23; i++) {

		sf::Texture tex;
		std::string fichier;
		if ( i < 10 ) {
			fichier = "ma0"
					+ std::to_string(i);
		} else {
			fichier = "ma"
					+ std::to_string(i);
		}

		if (!tex.loadFromFile(path+"/"+fichier+".png")) {
			std::cerr << "Unable to load marker " << folder;
			throw std::runtime_error("Unable to load marker " + folder);
		}
		textures.insert({fichier, tex});
	}
}

sf::Sprite Marker::getSprite(Etat etat, int frame) {

	int lower;
	int upper;
	switch(etat) {
		case MISS:
			lower = 16;
			upper = 32;
			break;
		default:
			lower = 0;
			upper = 15;
	}

	if (!(frame >= lower and frame <= upper)) {
		std::cerr << "Requested access to a non-existent marker frame : " << frame;
		throw std::runtime_error("Requested access to a non-existent marker frame : " +std::to_string(frame));
	}

	sf::Sprite sprite;
	std::string tex_key;
	switch (etat) {
		case APPROCHE:
		case MISS:
			tex_key += "ma";
			break;
		case EARLY:
			tex_key += "h1";
			break;
		case GOOD:
			tex_key += "h2";
			break;
		case GREAT:
			tex_key += "h3";
			break;
		case PERFECT:
			tex_key += "h4";
			break;
	}
	if (frame < 10) {
		tex_key += "0";
	}

	sprite.setTexture(textures[tex_key+std::to_string(frame)]);

	return sprite;

}
