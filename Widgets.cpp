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
	graph.setSmooth(true);

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

void Widgets::LinearView::setZoom(int newZoom) {
	zoom = std::clamp(newZoom,-5,5);
}

void Widgets::LinearView::update(std::optional<Chart> chart, sf::Time playbackPosition, float ticksAtPlaybackPosition, float BPM, int resolution, ImVec2 size) {

	int x = std::max(140, static_cast<int>(size.x));
	int y = std::max(140, static_cast<int>(size.y));

	if (!view.create(static_cast<unsigned int>(x), static_cast<unsigned int>(y))) {
		std::cerr << "Unable to create LinearView's RenderTexture";
		throw std::runtime_error("Unable to create LinearView's RenderTexture");
	}
	view.setSmooth(true);
	view.clear(sf::Color::Transparent);

	if (chart) {

		AffineTransform<float> SecondsToTicks(playbackPosition.asSeconds()-(60.f/BPM)/timeFactor(), playbackPosition.asSeconds(), ticksAtPlaybackPosition-resolution/timeFactor(), ticksAtPlaybackPosition);
		AffineTransform<float> PixelsToSeconds(-25.f, 75.f, playbackPosition.asSeconds()-(60.f/BPM)/timeFactor(), playbackPosition.asSeconds());
		AffineTransform<float> PixelsToTicks(-25.f, 75.f, ticksAtPlaybackPosition-resolution/timeFactor(), ticksAtPlaybackPosition);


		// Draw the beat lines and numbers
		int next_beat_tick =  ((1 + (static_cast<int>(PixelsToTicks.transform(0.f))+resolution)/resolution) * resolution) - resolution;
		int next_beat = std::max(0,next_beat_tick/resolution);
		next_beat_tick = next_beat*resolution;
		float next_beat_line_y = PixelsToTicks.backwards_transform(static_cast<float>(next_beat_tick));

		sf::RectangleShape beat_line(sf::Vector2f(static_cast<float>(x)-80.f,1.f));

		sf::Text beat_number;
		beat_number.setFont(beat_number_font);
		beat_number.setCharacterSize(15);
		beat_number.setFillColor(sf::Color::White);
		std::stringstream ss;

		while (next_beat_line_y < static_cast<float>(y)) {

			if (next_beat%4 == 0) {

				beat_line.setFillColor(sf::Color::White);
				beat_line.setPosition({50.f,next_beat_line_y});
				view.draw(beat_line);


				ss.str(std::string());
				ss << next_beat/4;
				beat_number.setString(ss.str());
				sf::FloatRect textRect = beat_number.getLocalBounds();
				beat_number.setOrigin(textRect.left + textRect.width, textRect.top  + textRect.height/2.f);
				beat_number.setPosition({40.f, next_beat_line_y});
				view.draw(beat_number);

			} else {

				beat_line.setFillColor(sf::Color(255,255,255,127));
				beat_line.setPosition({50.f,next_beat_line_y});
				view.draw(beat_line);

			}

			next_beat_tick += resolution;
			next_beat += 1;
			next_beat_line_y = PixelsToTicks.backwards_transform(static_cast<float>(next_beat_tick));
		}

		// Draw the notes;
		float note_width = (static_cast<float>(x)-80.f)/16.f;
		note_rect.setSize({note_width,6.f});
		sf::FloatRect note_bounds = note_rect.getLocalBounds();
		note_rect.setOrigin(note_bounds.left + note_bounds.width/2.f, note_bounds.top  + note_bounds.height/2.f);

		float collision_zone_size = 10.f*(60.f/BPM)*100.f*timeFactor();
		note_collision_zone.setSize({(static_cast<float>(x)-80.f)/16.f-2.f,collision_zone_size});
		sf::FloatRect collision_zone_bounds = note_collision_zone.getLocalBounds();
		note_collision_zone.setOrigin(collision_zone_bounds.left + collision_zone_bounds.width/2.f, collision_zone_bounds.top  + collision_zone_bounds.height/2.f);

		int lower_bound_ticks = std::max(0,static_cast<int>(SecondsToTicks.transform(PixelsToSeconds.transform(0.f)-0.5f)));
		int upper_bound_ticks = std::max(0,static_cast<int>(SecondsToTicks.transform(PixelsToSeconds.transform(static_cast<float>(y))+0.5f)));

		auto lower_note = chart->Notes.lower_bound(Note(0,lower_bound_ticks));
		auto upper_note = chart->Notes.upper_bound(Note(15,upper_bound_ticks));

		if (lower_note != chart->Notes.end()) {
			for (auto note = lower_note; note != chart->Notes.end() and note != upper_note; ++note) {
				float note_x = 50.f+note_width*(note->getPos()+0.5f);
				float note_y = PixelsToTicks.backwards_transform(static_cast<float>(note->getTiming()));
				note_rect.setPosition(note_x,note_y);
				note_collision_zone.setPosition(note_x,note_y);
				view.draw(note_rect);
				view.draw(note_collision_zone);
			}
		}

		// Draw the cursor
		cursor.setSize(sf::Vector2f(static_cast<float>(x)-76.f,4.f));
		view.draw(cursor);

	}

}

Widgets::LinearView::LinearView() {

	if (!beat_number_font.loadFromFile(font_path)) {
		std::cerr << "Unable to load " << font_path;
		throw std::runtime_error("Unable to load" + font_path);
	}
	cursor.setFillColor(sf::Color(66,150,250,200));
	cursor.setOrigin(0.f,2.f);
	cursor.setPosition({48.f,75.f});

	note_rect.setFillColor(sf::Color(230,179,0,255));
	note_collision_zone.setFillColor(sf::Color(230,179,0,127));
}
