#pragma once

#include <filesystem>

#include <toml++/toml.h>

#include "colors.hpp"
#include "sizes.hpp"
#include "marker.hpp"
#include "widgets/lane_order.hpp"


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

    linear_view::Colors load_linear_view_colors_from_v1_0_0_table(const toml::table& linear_view);
    void dump_linear_view_colors_as_v1_0_0(const linear_view::Colors& colors, toml::table& linear_view);

    linear_view::Sizes load_linear_view_sizes_from_v1_0_0_table(const toml::table& linear_view);
    void dump_linear_view_sizes_as_v1_0_0(const linear_view::Sizes& sizes, toml::table& linear_view);

    linear_view::LaneOrder load_linear_view_lane_order_from_v1_0_0_table(const toml::table& linear_view);
    void dump_linear_view_lane_order_as_v1_0_0(const linear_view::LaneOrder& lane_order, toml::table& linear_view);

    struct LinearView {
        linear_view::Colors colors;
        linear_view::Sizes sizes;
        linear_view::LaneOrder lane_order;

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