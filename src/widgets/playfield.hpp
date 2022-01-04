#ifndef FEIS_PLAYFIELD_H
#define FEIS_PLAYFIELD_H

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <string>

#include "../ln_marker.hpp"
#include "../marker.hpp"
#include "../note.hpp"

class Playfield {
public:
    Playfield(std::filesystem::path assets_folder);
    sf::Texture base_texture;
    sf::Sprite button;
    sf::Sprite button_pressed;
    sf::Sprite note_selected;
    sf::Sprite note_collision;

    sf::RenderTexture markerLayer;
    sf::Sprite markerSprite;

    LNMarker longNoteMarker;
    sf::RenderTexture longNoteLayer;
    sf::Sprite LNSquareBackgroud;
    sf::Sprite LNSquareOutline;
    sf::Sprite LNSquareHighlight;
    sf::Sprite LNTail;
    sf::Sprite LNTriangle;

    void resize(unsigned int width);

    void drawLongNote(
        const Note& note,
        const sf::Time& playbackPosition,
        const float& ticksAtPlaybackPosition,
        const float& BPM,
        const int& resolution);

    void drawLongNote(
        const Note& note,
        const sf::Time& playbackPosition,
        const float& ticksAtPlaybackPosition,
        const float& BPM,
        const int& resolution,
        Marker& marker,
        MarkerEndingState& markerEndingState);

private:
    const std::filesystem::path texture_path;
};

#endif  // FEIS_PLAYFIELD_H
