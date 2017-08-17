//
// Created by Syméon on 17/08/2017.
//

#ifndef FEIS_SCREEN_H
#define FEIS_SCREEN_H

#include <SFML/Graphics.hpp>
#include "Marker.h"

class Screen {

public:

	virtual void run(sf::RenderWindow& window) = 0;

};

class Ecran_attente : public Screen {

public:

	Ecran_attente();
	void run(sf::RenderWindow& window);

private:

	sf::Color gris_de_fond;
	sf::Texture tex_FEIS_logo;
	sf::Sprite FEIS_logo;

};

class Ecran_edition : public Screen {

public:

	Ecran_edition();
	void run(sf::RenderWindow& window);

private:

	sf::Color couleur_de_fond;
	Marker marker;

};

#endif //FEIS_SCREEN_H
