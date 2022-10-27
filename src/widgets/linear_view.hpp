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

struct ButtonColors {
    sf::Color text;
    sf::Color button;
    sf::Color hover;
    sf::Color active;
    sf::Color border;
};

struct RectangleColors {
    sf::Color fill;
    sf::Color border;
};

struct SelectionRectangle {
    sf::Vector2f start = {-1, -1};
    sf::Vector2f end = {-1, -1};

    void reset();
};

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
    sf::Color cursor_color = {66, 150, 250, 170};
    RectangleColors tab_selection_colors = {
        .fill = {153, 255, 153, 92},
        .border = {153, 255, 153, 189}
    };
    sf::Color normal_tap_note_color = {255, 213, 0};
    sf::Color conflicting_tap_note_color = {255, 167, 0};
    sf::Color normal_collision_zone_color = {230, 179, 0, 80};
    sf::Color conflicting_collision_zone_color = {255, 0, 0, 145};
    sf::Color normal_long_note_color = {255, 90, 0, 223};
    sf::Color conflicting_long_note_color = {255, 26, 0};
    sf::Color selected_note_fill = {255, 255, 255, 127};
    sf::Color selected_note_outline = sf::Color::White;
    sf::Color measure_lines_color = sf::Color::White;
    sf::Color measure_numbers_color = sf::Color::White;
    sf::Color beat_lines_color = {255, 255, 255, 127};
    ButtonColors bpm_button_colors = {
        .text = {66, 150, 250},
        .button = sf::Color::Transparent,
        .hover = {66, 150, 250, 64},
        .active = {66, 150, 250, 127},
        .border = {109, 179, 251}
    };
    RectangleColors selection_rect_colors = {
        .fill = {144, 189, 255, 64},
        .border = {144, 189, 255}
    };

    int timeline_margin = 130;
    int cursor_height = 100;
    AffineTransform<Fraction> beats_to_pixels_proportional;

    void reload_transforms();

    int zoom = 0;

    SelectionRectangle selection_rectangle;
    bool started_selection_inside_window = false;
    bool any_bpm_button_hovered = false;
};

void draw_rectangle(
    ImDrawList* draw_list,
    const sf::Vector2f& pos,
    const sf::Vector2f& size,
    const sf::Vector2f& normalized_anchor,
    const sf::Color& fill,
    const std::optional<sf::Color>& outline = {}
);

void BPMButton(
    const better::SelectableBPMEvent& event,
    const sf::Vector2f& pos,
    const ButtonColors& colors
);

bool draw_selection_rect(
    ImDrawList* draw_list,
    sf::Vector2f& start_pos,
    sf::Vector2f& end_pos,
    const RectangleColors& colors,
    ImGuiMouseButton mouse_button = ImGuiMouseButton_Left
);

void select_everything_inside(const sf::Vector2f& rect);
