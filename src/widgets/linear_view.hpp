#pragma once

#include <filesystem>
#include <map>
#include <thread>

#include <imgui.h>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>

#include "../custom_sfml_audio/open_music.hpp"
#include "../better_timing.hpp"
#include "../chart_state.hpp"
#include "../toolbox.hpp"
#include "../config.hpp"
#include "../linear_view_sizes.hpp"
#include "../linear_view_colors.hpp"
#include "../utf8_sfml.hpp"
#include "../quantization_colors.hpp"
#include "better_note.hpp"
#include "lane_order.hpp"
#include "../waveform.hpp"

struct SelectionRectangle {
    sf::Vector2f start = {-1, -1};
    sf::Vector2f end = {-1, -1};

    void reset();
};

const std::map<unsigned int, sf::Color> reference_note_colors = {{
    {1, {255, 40, 40}},
    {2, {34, 140, 255}},
    {3, {156, 0, 254}},
    {4, {248, 236, 18}},
    {6, {255, 131, 189}},
    {8, {254, 135, 0}},
    {12, {0, 254, 207}},
    {16, {68, 254, 0}}
}};
const sf::Color reference_note_grey = {134, 110, 116};

namespace linear_view {
    struct ComputedSizes {
        int x;
        int y;
        float timeline_width;
        float timeline_left;
        float timeline_right;
        float cursor_y;
        float bpm_events_left;
        float note_width;
        float collizion_zone_width;
        float long_note_rect_width;
        sf::Vector2f note_size;
        sf::Vector2f selected_note_size;
        float cursor_width; 
        float cursor_left;
        sf::Vector2f cursor_size;
        sf::Vector2f cursor_pos;
    };

    ComputedSizes compute_sizes(
        const sf::Vector2f& window_size,
        linear_view::Sizes& size_settings
    );
}

class LinearView {
public:
    LinearView(std::filesystem::path assets, config::Config& config);

    struct DrawArgs {
        ImDrawList* draw_list;
        ChartState& chart_state;
        const waveform::Status& waveform_status;
        const better::Timing& timing;
        const Fraction& current_beat;
        const Fraction& last_editable_beat;
        const Fraction& snap;
        const sf::Vector2f& size;
        const sf::Vector2f& origin;
    };

    void draw(DrawArgs& args);

    void set_zoom(int zoom);
    void zoom_in() { set_zoom(zoom + 1); };
    void zoom_out() { set_zoom(zoom - 1); };
    float time_factor() { return std::pow(1.25, static_cast<double>(zoom)); };

    bool shouldDisplaySettings;

    void display_settings();

private:
    void draw_in_beats_mode(DrawArgs& args);
    void draw_in_waveform_mode(DrawArgs& args);
    void draw_tap_note(
        LinearView::DrawArgs& args,
        const linear_view::ComputedSizes& computed_sizes,
        const better::TapNote& tap_note,
        const sf::Vector2f note_pos,
        const sf::Time collision_zone,
        const float collision_zone_y,
        const float collision_zone_height
    );
    void draw_long_note(
        LinearView::DrawArgs& args,
        const linear_view::ComputedSizes& computed_sizes,
        const better::LongNote& long_note,
        const sf::Vector2f note_pos,
        const float long_note_rect_height,
        const sf::Time collision_zone,
        const float collision_zone_y,
        const float collision_zone_height
    );

    template<class T>
    void draw_notes(
        const sf::Time first_visible_second,
        const sf::Time last_visible_second,
        const ChartState& chart_state,
        const better::Timing& timing,
        const T& draw_one_note
    ) {
        const auto first_visible_collision_zone = timing.beats_at(first_visible_second - collision_zone * 0.5f);
        const auto last_visible_collision_zone = timing.beats_at(last_visible_second + collision_zone * 0.5f);
        chart_state.chart.notes->in(
            first_visible_collision_zone,
            last_visible_collision_zone,
            [&](const better::Notes::iterator& it){
                it->second.visit(draw_one_note);
            }
        );
    };

    template<class T>
    void draw_long_note_dummy(
        const ChartState& chart_state,
        const Fraction& snap,
        const T& draw_one_note
    ) {
        if (chart_state.long_note_being_created.has_value()) {
            draw_one_note(
                make_long_note_dummy_for_linear_view(
                    *chart_state.long_note_being_created,
                    snap
                )
            );
        }
    };

    void draw_cursor(
        ImDrawList* draw_list,
        const sf::Vector2f& origin,
        const linear_view::ComputedSizes& computed_sizes
    );
    void draw_time_selection(
        ImDrawList* draw_list,
        const sf::Vector2f& origin,
        const ChartState& chart_state,
        const linear_view::ComputedSizes& computed_sizes,
        const std::function<float(const Fraction&)> beats_to_absolute_pixels
    );
    void handle_mouse_selection(
        ImDrawList* draw_list,
        const sf::Vector2f& origin,
        ChartState& chart_state,
        const better::Timing& timing,
        const linear_view::ComputedSizes& computed_sizes,
        const std::function<Fraction(float)> absolute_pixels_to_beats
    );

    linear_view::Mode& mode;
    std::string mode_name();
    linear_view::Colors& colors;
    linear_view::Sizes& sizes;
    const sf::Time& collision_zone;

    AffineTransform<Fraction> beats_to_pixels_proportional;

    void reload_transforms();

    int& zoom;

    bool& use_quantization_colors;
    linear_view::QuantizationColors& quantization_colors;

    SelectionRectangle selection_rectangle;
    bool started_selection_inside_window = false;
    bool any_bpm_button_hovered = false;

    linear_view::LaneOrder& lane_order;
    std::string lane_order_name() const;
    std::optional<unsigned int> button_to_lane(const better::Position& button) const;
    std::optional<better::Position> lane_to_button(unsigned int lane) const;
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