//
// Created by Syméon on 17/08/2017.
//

#include "Widgets.h"


Widgets::BlankScreen::BlankScreen() : gris_de_fond(sf::Color(38,38,38)) {

	if(!tex_FEIS_logo.loadFromFile("assets/textures/FEIS_logo.png"))
	{
		throw std::string("Unable to load assets/textures/FEIS_logo.png");
	}
	tex_FEIS_logo.setSmooth(true);
	FEIS_logo.setTexture(tex_FEIS_logo);
	FEIS_logo.setColor(sf::Color(255, 255, 255, 32)); // un huitième opaque

}

void Widgets::BlankScreen::render(sf::RenderWindow& window) {
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

	note_selected.setTexture(base_texture);
	note_selected.setTextureRect({384,0,192,192});

	note_collision.setTexture(base_texture);
	note_collision.setTextureRect({576,0,192,192});

	if (!markerLayer.create(400,400)) {
		std::cerr << "Unable to create Playfield's markerLayer";
		throw std::runtime_error("Unable to create Playfield's markerLayer");
	}
	markerLayer.setSmooth(true);

	if (!longNoteLayer.create(400,400)) {
		std::cerr << "Unable to create Playfield's longNoteLayer";
		throw std::runtime_error("Unable to create Playfield's longNoteLayer");
	}
	longNoteLayer.setSmooth(true);

	LNSquareBackgroud.setTexture(*longNoteMarker.getSquareBackgroundTexture(0));
	LNSquareOutline.setTexture(*longNoteMarker.getSquareOutlineTexture(0));
	LNSquareHighlight.setTexture(*longNoteMarker.getSquareHighlightTexture(0));
	LNTail.setTexture(*longNoteMarker.getTailTexture(0));
	LNTriangle.setTexture(*longNoteMarker.getTriangleTexture(0));
}

void Widgets::Playfield::resize(unsigned int width) {

	if (longNoteLayer.getSize() != sf::Vector2u(width,width)) {
		if (!longNoteLayer.create(width, width)) {
			std::cerr << "Unable to resize Playfield's longNoteLayer";
			throw std::runtime_error("Unable to resize Playfield's longNoteLayer");
		}
		longNoteLayer.setSmooth(true);
	}

	longNoteLayer.clear(sf::Color::Transparent);

	if (markerLayer.getSize() != sf::Vector2u(width,width)) {
		if (!markerLayer.create(width, width)) {
			std::cerr << "Unable to resize Playfield's markerLayer";
			throw std::runtime_error("Unable to resize Playfield's markerLayer");
		}
		markerLayer.setSmooth(true);
	}

	markerLayer.clear(sf::Color::Transparent);

}

void Widgets::Playfield::drawLongNote(const Note &note, const sf::Time &playbackPosition,
									  const float &ticksAtPlaybackPosition, const float &BPM, const int &resolution) {

	float squareSize = static_cast<float>(longNoteLayer.getSize().x) / 4;

	AffineTransform<float> SecondsToTicksProportional(0.f, (60.f / BPM), 0.f, resolution);

	float note_offset = SecondsToTicksProportional.backwards_transform(ticksAtPlaybackPosition - note.getTiming());
	auto frame = static_cast<long long int>(std::floor(note_offset * 30.f));
	int x = note.getPos() % 4;
	int y = note.getPos() / 4;

	float tail_end_in_seconds = SecondsToTicksProportional.backwards_transform(note.getTiming() + note.getLength());
	float tail_end_offset = playbackPosition.asSeconds() - tail_end_in_seconds;

	if (playbackPosition.asSeconds() < tail_end_in_seconds) {

		// Before or During the long note
		auto tail_tex = longNoteMarker.getTailTexture(note_offset);
		if (tail_tex) {

			auto triangle_distance = static_cast<float>((note.getTail_pos() / 4) + 1);

			AffineTransform<float> OffsetToTriangleDistance(
				0.f,
				SecondsToTicksProportional.backwards_transform(note.getLength()),
				triangle_distance,
				0.f
			);

			LNTail.setTexture(*tail_tex, true);
			auto LNTriangle_tex = longNoteMarker.getTriangleTexture(note_offset);
			if (LNTriangle_tex) {
				LNTriangle.setTexture(*LNTriangle_tex, true);
			}
			auto LNSquareBackgroud_tex = longNoteMarker.getSquareBackgroundTexture(note_offset);
			if (LNSquareBackgroud_tex) {
				LNSquareBackgroud.setTexture(*LNSquareBackgroud_tex, true);
			}
			auto LNSquareOutline_tex = longNoteMarker.getSquareOutlineTexture(note_offset);
			if (LNSquareOutline_tex) {
				LNSquareOutline.setTexture(*LNSquareOutline_tex, true);
			}
			auto LNSquareHighlight_tex = longNoteMarker.getSquareHighlightTexture(note_offset);
			if (LNSquareHighlight_tex) {
				LNSquareHighlight.setTexture(*LNSquareHighlight_tex, true);
			}

			auto rect = LNTail.getTextureRect();
			float tail_length_factor;

			if (frame < 8) {
				// Before the note : tail goes from triangle tip to note edge
				tail_length_factor = std::max(0.f, OffsetToTriangleDistance.clampedTransform(note_offset) - 1.f);
			} else {
				// During the note : tail goes from half of the triangle base to note edge
				tail_length_factor = std::max(0.f, OffsetToTriangleDistance.clampedTransform(note_offset) - 0.5f);
			}

			rect.height = static_cast<int>(rect.height * tail_length_factor);
			LNTail.setTextureRect(rect);
			LNTail.setOrigin(rect.width / 2.f, -rect.width / 2.f);
			LNTail.setRotation(90.f * ((note.getTail_pos() + 2) % 4));

			rect = LNTriangle.getTextureRect();
			LNTriangle.setOrigin(rect.width / 2.f,
								 rect.width * (0.5f + OffsetToTriangleDistance.clampedTransform(note_offset)));
			LNTriangle.setRotation(90.f * (note.getTail_pos() % 4));

			rect = LNSquareBackgroud.getTextureRect();
			LNSquareBackgroud.setOrigin(rect.width / 2.f, rect.height / 2.f);
			LNSquareBackgroud.setRotation(90.f * (note.getTail_pos() % 4));

			rect = LNSquareOutline.getTextureRect();
			LNSquareOutline.setOrigin(rect.width / 2.f, rect.height / 2.f);
			LNSquareOutline.setRotation(90.f * (note.getTail_pos() % 4));

			rect = LNSquareHighlight.getTextureRect();
			LNSquareHighlight.setOrigin(rect.width / 2.f, rect.height / 2.f);

			float scale = squareSize / rect.width;
			LNTail.setScale(scale, scale);
			LNTriangle.setScale(scale, scale);
			LNSquareBackgroud.setScale(scale, scale);
			LNSquareOutline.setScale(scale, scale);
			LNSquareHighlight.setScale(scale, scale);

			LNTail.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
			LNTriangle.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
			LNSquareBackgroud.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
			LNSquareOutline.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
			LNSquareHighlight.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);

			longNoteLayer.draw(LNTail);
			longNoteLayer.draw(LNSquareBackgroud);
			longNoteLayer.draw(LNSquareOutline);
			longNoteLayer.draw(LNTriangle);
			longNoteLayer.draw(LNSquareHighlight);

		}

	}

}

void Widgets::Playfield::drawLongNote(
	const Note &note,
	const sf::Time &playbackPosition,
	const float &ticksAtPlaybackPosition,
	const float& BPM,
	const int& resolution,
	Marker& marker,
	MarkerEndingState& markerEndingState
) {

	drawLongNote(note,playbackPosition,ticksAtPlaybackPosition,BPM,resolution);

	float squareSize = static_cast<float>(longNoteLayer.getSize().x) / 4;

	AffineTransform<float> SecondsToTicksProportional(0.f, (60.f / BPM), 0.f, resolution);

	float note_offset = SecondsToTicksProportional.backwards_transform(ticksAtPlaybackPosition - note.getTiming());
	int x = note.getPos() % 4;
	int y = note.getPos() / 4;

	float tail_end_in_seconds = SecondsToTicksProportional.backwards_transform(note.getTiming() + note.getLength());
	float tail_end_offset = playbackPosition.asSeconds() - tail_end_in_seconds;

	if (playbackPosition.asSeconds() < tail_end_in_seconds) {

		// Before or During the long note
		// Display the beginning marker
		auto t = marker.getSprite(markerEndingState, note_offset);
		if (t) {
			float scale = squareSize / t->get().getSize().x;
			markerSprite.setTexture(*t, true);
			markerSprite.setScale(scale, scale);
			markerSprite.setPosition(x * squareSize, y * squareSize);
			markerLayer.draw(markerSprite);
		}

	} else {

		// After long note end : Display the ending marker
		if (tail_end_offset >= 0.0f) {
			auto t = marker.getSprite(markerEndingState, tail_end_offset);
			if (t) {
				float scale = squareSize / t->get().getSize().x;
				markerSprite.setTexture(*t, true);
				markerSprite.setScale(scale, scale);
				markerSprite.setPosition(x * squareSize, y * squareSize);
				markerLayer.draw(markerSprite);
			}
		}
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

	selection.setFillColor(sf::Color(153,255,153,92));
	selection.setOutlineColor(sf::Color(153,255,153,189));
	selection.setOutlineThickness(1.f);

	note_rect.setFillColor(sf::Color(255,213,0,255));

	note_selected.setFillColor(sf::Color(255,255,255,200));
	note_selected.setOutlineThickness(1.f);

	note_collision_zone.setFillColor(sf::Color(230,179,0,127));

	long_note_rect.setFillColor(sf::Color(255,90,0,223));
	long_note_collision_zone.setFillColor(sf::Color(230,179,0,127));

}

void Widgets::LinearView::update(const std::optional<Chart_with_History> chart, const sf::Time &playbackPosition,
								 const float &ticksAtPlaybackPosition, const float &BPM, const int &resolution,
								 const ImVec2 &size) {

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
		AffineTransform<float> SecondsToTicksProportional(0.f,(60.f/BPM),0.f,resolution);
		AffineTransform<float> PixelsToSeconds(-25.f, 75.f, playbackPosition.asSeconds()-(60.f/BPM)/timeFactor(), playbackPosition.asSeconds());
		AffineTransform<float> PixelsToSecondsProprotional(0.f, 100.f, 0.f, (60.f/BPM)/timeFactor());
		AffineTransform<float> PixelsToTicks(-25.f, 75.f, ticksAtPlaybackPosition-resolution/timeFactor(), ticksAtPlaybackPosition);


		/*
		 * Draw the beat lines and numbers
		 */

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

		/*
		 * Draw the notes
		 */

		// Size & center the shapes
		float note_width = (static_cast<float>(x)-80.f)/16.f;
		note_rect.setSize({note_width,6.f});
		Toolbox::center(note_rect);

		note_selected.setSize({note_width+2.f,8.f});
		Toolbox::center(note_selected);

		float collision_zone_size = PixelsToSecondsProprotional.backwards_transform(1.f);
		note_collision_zone.setSize({(static_cast<float>(x)-80.f)/16.f-2.f,collision_zone_size});
		Toolbox::center(note_collision_zone);

		long_note_collision_zone.setSize({(static_cast<float>(x)-80.f)/16.f-2.f,collision_zone_size});
		Toolbox::center(long_note_collision_zone);

		// Find the notes that need to be displayed
		int lower_bound_ticks = std::max(0,static_cast<int>(SecondsToTicks.transform(PixelsToSeconds.transform(0.f)-0.5f)));
		int upper_bound_ticks = std::max(0,static_cast<int>(SecondsToTicks.transform(PixelsToSeconds.transform(static_cast<float>(y))+0.5f)));

		auto notes = chart->ref.getVisibleNotesBetween(lower_bound_ticks,upper_bound_ticks);
		auto currentLongNote = chart->makeCurrentLongNote();
		if (currentLongNote) {
			notes.insert(*currentLongNote);
		}

		for (auto& note : notes) {

			float note_x = 50.f+note_width*(note.getPos()+0.5f);
			float note_y = PixelsToTicks.backwards_transform(static_cast<float>(note.getTiming()));
			note_rect.setPosition(note_x,note_y);
			note_selected.setPosition(note_x,note_y);
			note_collision_zone.setPosition(note_x,note_y);

			if (note.getLength() != 0) {

				float tail_size = PixelsToSecondsProprotional.backwards_transform(SecondsToTicksProportional.backwards_transform(note.getLength()));
				float long_note_collision_size = collision_zone_size + tail_size;
				long_note_collision_zone.setSize({(static_cast<float>(x)-80.f)/16.f-2.f,collision_zone_size});
				Toolbox::center(long_note_collision_zone);
				long_note_collision_zone.setSize({(static_cast<float>(x)-80.f)/16.f-2.f,long_note_collision_size});
				long_note_collision_zone.setPosition(note_x,note_y);

				view.draw(long_note_collision_zone);


				float tail_width = .75f*(static_cast<float>(x)-80.f)/16.f;
				long_note_rect.setSize({tail_width,tail_size});
				sf::FloatRect long_note_bounds = long_note_rect.getLocalBounds();
				long_note_rect.setOrigin(long_note_bounds.left + long_note_bounds.width/2.f, long_note_bounds.top);
				long_note_rect.setPosition(note_x,note_y);

				view.draw(long_note_rect);

			} else {
				view.draw(note_collision_zone);
			}

			view.draw(note_rect);


			if (chart->selectedNotes.find(note) != chart->selectedNotes.end()) {
				view.draw(note_selected);
			}
		}

		/*
		 * Draw the cursor
		 */
		cursor.setSize({static_cast<float>(x)-76.f,4.f});
		view.draw(cursor);

		/*
		 * Draw the timeSelection
		 */

		selection.setSize({static_cast<float>(x)-80.f,0.f});
		if (std::holds_alternative<unsigned int>(chart->timeSelection)) {
			unsigned int ticks = std::get<unsigned int>(chart->timeSelection);
			float selection_y = PixelsToTicks.backwards_transform(static_cast<float>(ticks));
			if (selection_y > 0.f and selection_y < static_cast<float>(y)) {
				selection.setPosition(50.f,selection_y);
				view.draw(selection);
			}
		} else if (std::holds_alternative<TimeSelection>(chart->timeSelection)) {
			const auto& ts = std::get<TimeSelection>(chart->timeSelection);
			float selection_start_y = PixelsToTicks.backwards_transform(static_cast<float>(ts.start));
			float selection_end_y = PixelsToTicks.backwards_transform(static_cast<float>(ts.start+ts.duration));
			if (
				(selection_start_y > 0.f and selection_start_y < static_cast<float>(y)) or
				(selection_end_y > 0.f and selection_end_y < static_cast<float>(y))
			) {
				selection.setSize({static_cast<float>(x)-80.f,selection_end_y-selection_start_y});
				selection.setPosition(50.f,selection_start_y);
				view.draw(selection);
			}
		}

	}

}

void Widgets::LinearView::setZoom(int newZoom) {
	zoom = std::clamp(newZoom,-5,5);
}

void Widgets::LinearView::displaySettings() {
	if (ImGui::Begin("Linear View Settings",&shouldDisplaySettings)) {
		Toolbox::editFillColor("Cursor", cursor);
		Toolbox::editFillColor("Note", note_rect);
		if(Toolbox::editFillColor("Note Collision Zone", note_collision_zone)) {
			long_note_collision_zone.setFillColor(note_collision_zone.getFillColor());
		}
		Toolbox::editFillColor("Long Note Tail", long_note_rect);
	}
	ImGui::End();
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
