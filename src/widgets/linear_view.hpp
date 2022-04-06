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
        const sf::Time& playback_position,
        const ImVec2& size
    );

    void setZoom(int zoom);
    void zoom_in() { setZoom(zoom + 1); };
    void zoom_out() { setZoom(zoom - 1); };
    float timeFactor() { return std::pow(1.25f, static_cast<float>(zoom)); };

    bool shouldDisplaySettings;

    void displaySettings();

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
