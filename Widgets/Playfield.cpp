//
// Created by symeon on 06/04/19.
//

#include "Playfield.h"
#include "../Toolbox.h"

Playfield::Playfield() {

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

void Playfield::resize(unsigned int width) {

	if (longNoteLayer.getSize() != sf::Vector2u(width, width)) {
		if (!longNoteLayer.create(width, width)) {
			std::cerr << "Unable to resize Playfield's longNoteLayer";
			throw std::runtime_error("Unable to resize Playfield's longNoteLayer");
		}
		longNoteLayer.setSmooth(true);
	}

	longNoteLayer.clear(sf::Color::Transparent);

	if (markerLayer.getSize() != sf::Vector2u(width, width)) {
		if (!markerLayer.create(width, width)) {
			std::cerr << "Unable to resize Playfield's markerLayer";
			throw std::runtime_error("Unable to resize Playfield's markerLayer");
		}
		markerLayer.setSmooth(true);
	}

	markerLayer.clear(sf::Color::Transparent);

}

void Playfield::drawLongNote(const Note &note, const sf::Time &playbackPosition,
                             const float &ticksAtPlaybackPosition, const float &BPM, const int &resolution) {

	float squareSize = static_cast<float>(longNoteLayer.getSize().x) / 4;

	AffineTransform<float> SecondsToTicksProportional(0.f, (60.f / BPM), 0.f, resolution);
    AffineTransform<float> SecondsToTicks(playbackPosition.asSeconds()-(60.f/BPM), playbackPosition.asSeconds(), ticksAtPlaybackPosition-resolution, ticksAtPlaybackPosition);

	float note_offset = SecondsToTicksProportional.backwards_transform(ticksAtPlaybackPosition - note.getTiming());
	auto frame = static_cast<long long int>(std::floor(note_offset * 30.f));
	int x = note.getPos() % 4;
	int y = note.getPos() / 4;

	float tail_end_in_seconds = SecondsToTicks.backwards_transform(note.getTiming() + note.getLength());
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

void Playfield::drawLongNote(
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
    AffineTransform<float> SecondsToTicks(playbackPosition.asSeconds()-(60.f/BPM), playbackPosition.asSeconds(), ticksAtPlaybackPosition-resolution, ticksAtPlaybackPosition);

	float note_offset = SecondsToTicksProportional.backwards_transform(ticksAtPlaybackPosition - note.getTiming());
	int x = note.getPos() % 4;
	int y = note.getPos() / 4;

	float tail_end_in_seconds = SecondsToTicks.backwards_transform(note.getTiming() + note.getLength());
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