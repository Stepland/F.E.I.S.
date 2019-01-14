//
// Created by Syméon on 17/08/2017.
//

#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Marker.h"

namespace Widgets {
	class Ecran_attente {

	public:

		Ecran_attente();
		void render(sf::RenderWindow &window);

	private:

		sf::Color gris_de_fond;
		sf::Texture tex_FEIS_logo;
		sf::Sprite FEIS_logo;

	};

	class Playfield {

	public:

		Playfield();

		Marker marker;
		MarkerEndingState markerEndingState;
		sf::Texture button;
		sf::Texture button_pressed;

	private:
		std::string button_path = "assets/textures/edit_textures/game_front_edit_tex_1.tex.png";
	};
}
