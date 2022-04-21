#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <filesystem>

#include "../better_timing.hpp"
#include "../chart_state.hpp"
#include "../toolbox.hpp"
#include "imgui.h"

class LinearView {
public:
    LinearView(std::filesystem::path assets);

    void draw(
        ImDrawList* draw_list,
        const ChartState& chart_state,
        const better::Timing& timing,
        const Fraction& current_beat,
        const Fraction& last_editable_beat,
        const Fraction& snap,
        const sf::Vector2f& size,
        const sf::Vector2f& origin
    );

    void set_zoom(int zoom);
    void zoom_in() { set_zoom(zoom + 1); };
    void zoom_out() { set_zoom(zoom - 1); };
    float time_factor() { return std::pow(1.25, static_cast<double>(zoom)); };

    bool shouldDisplaySettings;

    void display_settings();

private:
    sf::Color bpm_text_color = {66, 150, 250};
    sf::Color cursor_color = {66, 150, 250, 170};
    sf::Color tab_selection_fill = {153, 255, 153, 92};
    sf::Color tab_selection_outline = {153, 255, 153, 189};
    sf::Color tap_note_color = {255, 213, 0};
    sf::Color long_note_color = {255, 90, 0, 223};
    sf::Color selected_note_fill = {255, 255, 255, 200};
    sf::Color selected_note_outline = sf::Color::White;
    sf::Color note_collision_zone_color = {230, 179, 0, 80};

    int timeline_margin = 130;
    int cursor_height = 100;
    AffineTransform<Fraction> beats_to_pixels_proportional;

    void reload_transforms();

    int zoom = 0;
};

void draw_rectangle(
    ImDrawList* draw_list,
    const sf::Vector2f& pos,
    const sf::Vector2f& size,
    const sf::Vector2f& normalized_anchor,
    const sf::Color& fill,
    const std::optional<sf::Color>& outline = {}
);
