#pragma once

#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <toml++/toml.h>

toml::array dump_color(const sf::Color& color);

template<class toml_node>
std::optional<sf::Uint8> parse_uint8(const toml_node& node) {
    const auto raw_value_opt = node.template value<int>();
    if (not raw_value_opt) {
        return {};
    }
    const auto raw_value = *raw_value_opt;
    if (raw_value < 0 or raw_value > 255) {
        return {};
    }
    return static_cast<sf::Uint8>(raw_value);
}

template<class toml_node>
std::optional<sf::Color> parse_color(const toml_node& node) {
    if (not node.is_array()) {
        return {};
    }
    const auto array = node.template ref<toml::array>();
    if (array.size() != 4) {
        return {};
    }
    const auto r = parse_uint8(array[0]);
    if (not r) {
        return {};
    }
    const auto g = parse_uint8(array[1]);
    if (not g) {
        return {};
    }
    const auto b = parse_uint8(array[2]);
    if (not b) {
        return {};
    }
    const auto a = parse_uint8(array[3]);
    if (not a) {
        return {};
    }
    return sf::Color(*r, *g, *b, *a);
}

template<class toml_node>
void load_color(const toml_node& node, sf::Color& color) {
    const auto parsed = parse_color(node);
    if (parsed) {
        color = *parsed;
    }
}