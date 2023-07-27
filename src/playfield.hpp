#pragma once

#include <string>

#include <imgui-SFML.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include "better_note.hpp"
#include "better_timing.hpp"
#include "config.hpp"
#include "ln_marker.hpp"
#include "marker.hpp"
#include "utf8_sfml_redefinitions.hpp"

class Playfield {
public:
    Playfield(std::filesystem::path assets_folder);
    feis::Texture button_texture;
    feis::Texture note_selected_texture;
    feis::Texture note_collision_texture;
    sf::Sprite button;
    sf::Sprite note_selected;
    sf::Sprite note_collision;

    sf::RenderTexture long_note_marker_layer;
    sf::RenderTexture chord_marker_layer;
    sf::RenderTexture note_numbers_layer;

    struct LongNote {
        template<typename ...Ts>
        LongNote(Ts&&... Args) : marker(std::forward<Ts>(Args)...) {};

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
        const sf::Time& playback_position,
        const better::Timing& timing,
        const Marker& marker,
        const Judgement& marker_ending_state,
        const std::optional<config::Playfield>& config
    );

    void draw_chord_tap_note(
        const better::TapNote& note,
        const sf::Time& playbackPosition,
        const better::Timing& timing,
        const Marker& marker,
        const Judgement& markerEndingState,
        const config::Playfield& config
    );

    void draw_note_number(
        const better::Note& note,
        const sf::Time& playbackPosition,
        const better::Timing& timing,
        const std::map<Fraction, unsigned int>& note_numbers,
        const config::Playfield& config
    );

private:

    void draw_chord_tap_note(
        const sf::Time& offset,
        const better::Position& position,
        const Marker& marker,
        const Judgement& marker_ending_state,
        const config::Playfield& config
    );

    std::optional<feis::Shader> chord_tint_shader;

    feis::Font note_numbers_font;
    sf::Text note_number;
};
