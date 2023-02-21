#pragma once

#include <filesystem>
#include <map>

#include <imgui.h>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>

#include "../better_timing.hpp"
#include "../chart_state.hpp"
#include "../toolbox.hpp"
#include "../config.hpp"
#include "../linear_view_sizes.hpp"
#include "../linear_view_colors.hpp"
#include "quantization_colors.hpp"
#include "lane_order.hpp"
#include "widgets/waveform_view.hpp"

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

namespace vertical_view {
    struct LinearView {};
    class WaveformView {
    public:
        explicit WaveformView(const std::filesystem::path& audio, config::Config& config);
        void draw(const sf::Time current_time);
        void draw_settings();
    private:
        feis::HoldFileStreamMixin<sf::InputSoundFile> sound_file;
        std::atomic<bool> data_is_ready = false;
        std::vector<std::pair<unsigned int, Channels>> channels_per_chunk_size;
        std::jthread worker;
        void prepare_data();
    };
}

class VerticalView {
public:
    VerticalView(std::filesystem::path assets, config::Config& config);

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
    std::variant<vertical_view::LinearView, vertical_view::WaveformView> status;
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