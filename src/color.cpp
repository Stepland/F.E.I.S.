#include "color.hpp"


toml::array dump_color(const sf::Color& color) {
    return toml::array{color.r, color.g, color.b, color.a};
}