#pragma once

#include <SFML/System/Time.hpp>
#include <filesystem>

#include <toml++/toml.h>

#include "quantization_colors.hpp"
#include "linear_view_colors.hpp"
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

    private:
        void load_from_v1_0_0_table(const toml::table& tbl);
        toml::table dump_as_v1_0_0();

        std::filesystem::path config_path;
    };
}