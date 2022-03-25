#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>

#include "../chart_state.hpp"
#include "../time_selection.hpp"
#include "../toolbox.hpp"

class LinearView {
public:
    LinearView(std::filesystem::path assets);

    sf::RenderTexture view;

    void update(
        const ChartState& chart_state,
        const sf::Time& playbackPosition,
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

    float last_BPM = 120.0f;
    int last_resolution = 240;
    bool shouldReloadTransforms;

    float cursor_y = 75.f;
    AffineTransform<Fraction> beats_to_pixels;
    AffineTransform<Fraction> beats_to_pixels_proportional;

    void resize(unsigned int width, unsigned int height);

    void reloadTransforms(
        const sf::Time& playbackPosition,
        const float& ticksAtPlaybackPosition,
        const float& BPM,
        const int& resolution);

    int zoom = 0;
    const std::filesystem::path font_path;
};
