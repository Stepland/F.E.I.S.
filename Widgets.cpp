//
// Created by Syméon on 17/08/2017.
//

#include "Widgets.h"
#include "Toolbox.h"

Widgets::Ecran_attente::Ecran_attente() : gris_de_fond(sf::Color(38,38,38)) {

	if(!tex_FEIS_logo.loadFromFile("assets/textures/FEIS_logo.png"))
	{
		throw std::string("Unable to load assets/textures/FEIS_logo.png");
	}
	tex_FEIS_logo.setSmooth(true);
	FEIS_logo.setTexture(tex_FEIS_logo);
	FEIS_logo.setColor(sf::Color(255, 255, 255, 32)); // un huitième opaque

}

void Widgets::Ecran_attente::render(sf::RenderWindow& window) {
    // effacement de la fenêtre en noir
    window.clear(gris_de_fond);

    // c'est ici qu'on dessine tout
    FEIS_logo.setPosition(sf::Vector2f(static_cast<float>((window.getSize().x-tex_FEIS_logo.getSize().x)/2),
                                       static_cast<float>((window.getSize().y-tex_FEIS_logo.getSize().y)/2)));
    window.draw(FEIS_logo);
}

Widgets::Playfield::Playfield() {
	if(!base_texture.loadFromFile(texture_path)) {
		std::cerr << "Unable to load texture " << texture_path;
		throw std::runtime_error("Unable to load texture " + texture_path);
	}
	base_texture.setSmooth(true);

	button.setTexture(base_texture);
	button.setTextureRect({0,0,192,192});

	button_pressed.setTexture(base_texture);
	button_pressed.setTextureRect({192,0,192,192});

	note_collision.setTexture(base_texture);
	note_collision.setTextureRect({576,0,192,192});
}

Widgets::DensityGraph::DensityGraph() {

	if (!base_texture.loadFromFile(texture_path)) {
		std::cerr << "Unable to load texture " << texture_path;
		throw std::runtime_error("Unable to load texture " + texture_path);
	}
	base_texture.setSmooth(true);

	normal_square.setTexture(base_texture);
	normal_square.setTextureRect({456,270,6,6});

	collision_square.setTexture(base_texture);
	collision_square.setTextureRect({496,270,6,6});

}

void Widgets::DensityGraph::computeDensities(int height, float chartRuntime, Chart& chart, float BPM, int resolution) {

	auto ticksToSeconds = [BPM, resolution](int ticks) -> float {return (60.f * ticks)/(BPM * resolution);};
	int ticks_threshold = static_cast<int>((1.f/60.f)*BPM*resolution);

	last_height = height;

	// minus the slider cursor thiccccnesss
	int available_height = height - 10;
	int sections = (available_height + 1) / 5;
	densities.clear();

	if (sections >= 1) {

		densities.resize(sections,{0,false});

		float section_length = chartRuntime/sections;
		last_section_length = section_length;

		int maximum = 0;

		for (auto const& note : chart.Notes) {
			int section = static_cast<int>(ticksToSeconds(note.getTiming())/section_length);
			int density = (densities[section].density += 1);
			if (not densities[section].has_collisions) {
				densities[section].has_collisions = chart.is_colliding(note,ticks_threshold);
			}
			if (maximum < density) {
				maximum = density;
			}
		}

		for (auto& density_entry : densities) {
			if (density_entry.density > 1) {
				density_entry.density = static_cast<int>(static_cast<float>(density_entry.density)/maximum * 8.0);
			}
		}
	}

	updateGraphTexture();
}

void Widgets::DensityGraph::updateGraphTexture() {

	if (!graph.create(45, static_cast<unsigned int>(*last_height))) {
		std::cerr << "Unable to create DensityGraph's RenderTexture";
		throw std::runtime_error("Unable to create DensityGraph's RenderTexture");
	}
	graph_rect = {0.0, 0.0, 45.0, static_cast<float>(*last_height)};

	graph.clear(sf::Color::Transparent);
	unsigned int x = 2;
	unsigned int y = 4;

	unsigned int line = 0;

	for (auto const& density_entry : densities) {
		if (density_entry.has_collisions) {
			for (int col = 0; col < density_entry.density; ++col) {
				collision_square.setPosition(x+col*5,y+line*5);
				graph.draw(collision_square);
			}
		} else {
			for (int col = 0; col < density_entry.density; ++col) {
				normal_square.setPosition(x+col*5,y+line*5);
				graph.draw(normal_square);
			}
		}
		++line;
	}

}
