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
    Playfield();
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
    const std::string texture_path =
        "assets/textures/edit_textures/game_front_edit_tex_1.tex.png";
};

#endif  // FEIS_PLAYFIELD_H
