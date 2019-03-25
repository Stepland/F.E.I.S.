//
// Created by Syméon on 17/08/2017.
//

#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Marker.h"
#include "LNMarker.h"
#include "Chart.h"

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
		sf::Texture base_texture;
		sf::Sprite button;
		sf::Sprite button_pressed;
		sf::Sprite note_collision;
		LNMarker longNoteMarker;

	private:
		std::string texture_path = "assets/textures/edit_textures/game_front_edit_tex_1.tex.png";
	};

	class LinearView {

	public:

		LinearView();

		sf::RenderTexture view;
		sf::Font beat_number_font;
		sf::RectangleShape cursor;
		sf::RectangleShape note_rect;
		sf::RectangleShape note_collision_zone;

	    void update(std::optional<Chart> chart, sf::Time playbackPosition, float ticksAtPlaybackPosition, float BPM, int resolution, ImVec2 size);

        void setZoom(int zoom);
        void zoom_in() {setZoom(zoom+1);};
        void zoom_out() {setZoom(zoom-1);};
        float timeFactor() {return std::pow(1.25f,static_cast<float>(zoom));};

    private:

        int zoom = 0;
		std::string font_path = "assets/fonts/NotoSans-Medium.ttf";

	};

	class DensityGraph {

	public:

		struct density_entry {
			int density;
			bool has_collisions;
		};

		DensityGraph();
		sf::Texture base_texture;
		sf::Sprite normal_square;
		sf::Sprite collision_square;
		sf::RenderTexture graph;
		sf::FloatRect graph_rect;

		bool should_recompute = false;

		std::vector<DensityGraph::density_entry> densities;

		std::optional<int> last_height;
		std::optional<float> last_section_length;

		void computeDensities(int height, float chartRuntime, Chart& chart, float BPM, int resolution);
		void updateGraphTexture();

		/*
		void addAtTime(float seconds);
		void removeAtTime(float seconds);
		*/

	private:
		std::string texture_path = "assets/textures/edit_textures/game_front_edit_tex_1.tex.png";

	};
}
