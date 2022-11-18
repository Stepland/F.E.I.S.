#pragma once

#include <filesystem>

#include <toml++/toml.h>

#include "marker.hpp"
#include "widgets/linear_view.hpp"


namespace config {
    using node_view = toml::node_view<const toml::node>;

    toml::array dump_color(const sf::Color& color);
    std::optional<sf::Color> parse_color(const node_view& node);
    void load_color(const node_view& node, sf::Color& color);

    struct Marker {
        std::optional<std::filesystem::path> folder;
        std::optional<Judgement> ending_state;

        void load_from_v1_0_0_table(const toml::table& tbl);
        void dump_as_v1_0_0(toml::table& tbl);
    };

    struct LinearViewColors {
        sf::Color cursor = {66, 150, 250, 170};
        RectangleColors tab_selection = {
            .fill = {153, 255, 153, 92},
            .border = {153, 255, 153, 189}
        };
        sf::Color normal_tap_note = {255, 213, 0};
        sf::Color conflicting_tap_note = {255, 167, 0};
        sf::Color normal_collision_zone = {230, 179, 0, 80};
        sf::Color conflicting_collision_zone = {255, 0, 0, 145};
        sf::Color normal_long_note = {255, 90, 0, 223};
        sf::Color conflicting_long_note = {255, 26, 0};
        sf::Color selected_note_fill = {255, 255, 255, 127};
        sf::Color selected_note_outline = sf::Color::White;
        sf::Color measure_line = sf::Color::White;
        sf::Color measure_number = sf::Color::White;
        sf::Color beat_line = {255, 255, 255, 127};
        ButtonColors bpm_button = {
            .text = {66, 150, 250},
            .button = sf::Color::Transparent,
            .hover = {66, 150, 250, 64},
            .active = {66, 150, 250, 127},
            .border = {109, 179, 251}
        };
        RectangleColors selection_rect = {
            .fill = {144, 189, 255, 64},
            .border = {144, 189, 255}
        };

        void load_from_v1_0_0_table(const toml::table& linear_view);
        void dump_as_v1_0_0(toml::table& linear_view);
    };

    struct LinearView {
        LinearViewColors colors;

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

    private:
        void load_from_v1_0_0_table(const toml::table& tbl);
        toml::table dump_as_v1_0_0();

        std::filesystem::path config_path;
    };
}