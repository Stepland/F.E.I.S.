#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>
#include <filesystem>

#include <toml++/toml.h>

#include "generic_interval.hpp"
#include "quantization_colors.hpp"
#include "linear_view_colors.hpp"
#include "linear_view_mode.hpp"
#include "linear_view_sizes.hpp"
#include "marker.hpp"
#include "widgets/lane_order.hpp"


namespace config {
    struct Marker {
        std::optional<std::filesystem::path> folder;
        std::optional<Judgement> ending_state;

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    struct LinearView {
        linear_view::Mode mode;
        linear_view::Colors colors;
        linear_view::Sizes sizes;
        linear_view::LaneOrder lane_order;
        int zoom = 0;
        bool use_quantization_colors = false;
        linear_view::QuantizationColors quantization_colors;

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    struct Editor {
        sf::Time collision_zone = sf::seconds(1);
        bool show_free_buttons = false;

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    struct Sound {
        int music_volume = 10;
        bool beat_tick = false;
        int beat_tick_volume = 10;
        bool note_clap = false;
        int note_clap_volume = 10;
        bool clap_on_long_note_ends = false;
        bool distinct_chord_clap = false;

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    struct Windows {
        bool show_playfield = true;
        bool show_playfield_settings = false;
        bool show_file_properties = false;
        bool show_status = false;
        bool show_playback_status = true;
        bool show_timeline = true;
        bool show_chart_list = false;
        bool show_linear_view = false;
        bool show_sound_settings = false;
        bool show_editor_settings = false;
        bool show_history = false;
        bool show_new_chart_dialog = false;
        bool show_chart_properties = false;
        bool show_sync_menu = false;
        bool show_bpm_change_menu = false;

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    struct Playfield {
        bool color_chords = false;
        sf::Color chord_color = sf::Color{110, 200, 250, 255};
        float chord_color_mix_amount = 1.0f;
        bool show_note_numbers = false;
        float note_number_size = 0.6f;
        sf::Color note_number_color = sf::Color::White;
        float note_number_stroke_width = 0.5f;
        sf::Color note_number_stroke_color = sf::Color::Black;
        Interval<int> note_number_visibility_time_span = {0, 500};

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    /* RAII-style class that holds settings we wish to save on disk and saves
    them upon being destroyed */
    class Config {
    public:
        Config(const std::filesystem::path& settings);
        ~Config();

        const std::string version = "1.0.0";

        Marker marker;
        LinearView linear_view;
        Editor editor;
        Sound sound;
        Windows windows;
        Playfield playfield;

    private:
        void load_from_v1_0_0_table(const toml::table& tbl);
        toml::table dump_as_v1_0_0();

        std::filesystem::path config_path;
    };
}