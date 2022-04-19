#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>

#include "../better_timing.hpp"
#include "../chart_state.hpp"
#include "../toolbox.hpp"

class LinearView {
public:
    LinearView(std::filesystem::path assets);

    sf::RenderTexture view;

    void update(
        const ChartState& chart_state,
        const better::Timing& timing,
        const Fraction& current_beat,
        const Fraction& last_editable_beat,
        const Fraction& snap,
        const ImVec2& size
    );

    void set_zoom(int zoom);
    void zoom_in() { set_zoom(zoom + 1); };
    void zoom_out() { set_zoom(zoom - 1); };
    float time_factor() { return std::pow(1.25, static_cast<double>(zoom)); };

    bool shouldDisplaySettings;

    void display_settings();

private:
    sf::Font beat_number_font;
    sf::RectangleShape cursor;
    sf::RectangleShape selection;
    sf::RectangleShape tap_note_rect;
    sf::RectangleShape long_note_rect;
    sf::RectangleShape note_selected;
    sf::RectangleShape note_collision_zone;

    float cursor_y = 75.f;
    AffineTransform<Fraction> beats_to_pixels_proportional;

    void resize(unsigned int width, unsigned int height);

    void reload_transforms();

    int zoom = 0;
    const std::filesystem::path font_path;
};
