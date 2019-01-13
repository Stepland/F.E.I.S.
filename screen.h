//
// Created by Syméon on 17/08/2017.
//

#ifndef FEIS_SCREEN_H
#define FEIS_SCREEN_H

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include "Marker.h"
#include "EditorState.h"

class Screen {

public:

	virtual void render(sf::RenderWindow &window, EditorState editorState) = 0;

};

class Ecran_attente {

public:

	Ecran_attente();
	void render(sf::RenderWindow &window);

private:

	sf::Color gris_de_fond;
	sf::Texture tex_FEIS_logo;
	sf::Sprite FEIS_logo;

};

class Ecran_edition : public Screen {

public:

	Ecran_edition();
	void render(sf::RenderWindow &window, EditorState editorState);

private:

	sf::Color couleur_de_fond;
	Marker marker;

};

#endif //FEIS_SCREEN_H
