#pragma once

#include <map>

#include <toml++/toml.h>
#include <SFML/Graphics/Color.hpp>

#include "special_numeric_types.hpp"

namespace linear_view {
    struct QuantizationColors {
        std::map<unsigned int, sf::Color> palette = {{
            {1, {255, 40, 40}},
            {2, {34, 140, 255}},
            {3, {156, 0, 254}},
            {4, {248, 236, 18}},
            {6, {255, 131, 189}},
            {8, {254, 135, 0}},
            {12, {0, 254, 207}},
            {16, {68, 254, 0}}
        }};
        sf::Color default_ = {156, 156, 156};
        sf::Color color_at_beat(const Fraction& time) const;

        void load_from_v1_0_0_table(const toml::table& linear_view_table);
        void dump_as_v1_0_0(toml::table& linear_view_table);
    };

    const QuantizationColors default_quantization_colors = {};
}