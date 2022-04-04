#pragma once

#include <string>

#include <imgui-SFML.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include "better_note.hpp"
#include "better_timing.hpp"
#include "ln_marker.hpp"
#include "marker.hpp"

class Playfield {
public:
    Playfield(std::filesystem::path assets_folder);
    sf::Texture base_texture;
    sf::Sprite button;
    sf::Sprite button_pressed;
    sf::Sprite note_selected;
    sf::Sprite note_collision;

    sf::RenderTexture marker_layer;
    sf::Sprite marker_sprite;

    struct LongNote {
        template<class... Args>
        LongNote(Args&& ...args) : marker(std::forward<Args>(args)...) {};

        LNMarker marker;
        sf::RenderTexture layer;
        sf::Sprite backgroud;
        sf::Sprite outline;
        sf::Sprite highlight;
        sf::Sprite tail;
        sf::Sprite triangle;
    };

    LongNote long_note;

    void resize(unsigned int width);

    void draw_tail_and_receptor(
        const better::LongNote& note,
        const sf::Time& playbackPosition,
        const better::Timing& timing
    );

    void draw_long_note(
        const better::LongNote& note,
        const sf::Time& playbackPosition,
        const better::Timing& timing,
        Marker& marker,
        MarkerEndingState& markerEndingState
    );

private:
    const std::filesystem::path texture_path;
};
