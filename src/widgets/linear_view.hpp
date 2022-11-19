#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <filesystem>

#include "../better_timing.hpp"
#include "../chart_state.hpp"
#include "../toolbox.hpp"
#include "config.hpp"
#include "imgui.h"

struct SelectionRectangle {
    sf::Vector2f start = {-1, -1};
    sf::Vector2f end = {-1, -1};

    void reset();
};

class LinearView {
public:
    LinearView(std::filesystem::path assets, config::LinearView& config);

    void draw(
        ImDrawList* draw_list,
        ChartState& chart_state,
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
    LinearViewColors& colors;

    int timeline_margin = 130;
    int cursor_height = 100;
    AffineTransform<Fraction> beats_to_pixels_proportional;

    void reload_transforms();

    int zoom = 0;

    SelectionRectangle selection_rectangle;
    bool started_selection_inside_window = false;
    bool any_bpm_button_hovered = false;

    struct LaneOrderPresets {
        struct Default {};
        struct Vertical {};
    };

    struct CustomLaneOrder {
        CustomLaneOrder();
        std::array<std::optional<unsigned int>, 16> lane_to_button;
        std::map<unsigned int, unsigned int> button_to_lane;
        std::string as_string;
        void cleanup_string();
        void update_from_string();
        void update_from_array();
    };

    std::variant<LaneOrderPresets::Default, LaneOrderPresets::Vertical, CustomLaneOrder> lane_order;
    std::string lane_order_name();
    std::optional<unsigned int> button_to_lane(const better::Position& button);
};

void draw_rectangle(
    ImDrawList* draw_list,
    const sf::Vector2f& pos,
    const sf::Vector2f& size,
    const sf::Vector2f& normalized_anchor,
    const sf::Color& fill,
    const std::optional<sf::Color>& outline = {}
);

bool BPMButton(
    const better::BPMEvent& event,
    bool selected,
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

void LaneOrderPreview(const std::array<std::optional<unsigned int>, 16>& order);