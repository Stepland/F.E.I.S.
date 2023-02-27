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
#include "quantization_colors.hpp"
#include "lane_order.hpp"

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
    namespace mode {
        struct Beats {};
        class Waveform {
        public:
            explicit Waveform(const std::filesystem::path& audio);
            void draw_waveform(
                ImDrawList* draw_list,
                const sf::Time current_time,
                int zoom
            );
        private:
            feis::HoldFileStreamMixin<sf::InputSoundFile> sound_file;
            std::atomic<bool> data_is_ready = false;
            std::vector<std::pair<unsigned int, Channels>> channels_per_chunk_size;
            std::jthread worker;
            void prepare_data();
        };

        namespace waveform {
        }
    }

    using Mode = std::variant<mode::Beats, mode::Waveform>;
}

class LinearView {
public:
    LinearView(std::filesystem::path assets, config::Config& config);

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
    linear_view::Mode mode;
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