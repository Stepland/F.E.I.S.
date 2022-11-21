#include "colors.hpp"
#include <SFML/Config.hpp>
#include <algorithm>
#include "hsluv/hsluv.h"

HSLuvColor color_to_hsluv(const sf::Color& rgb) {
    HSLuvColor hsl;
    rgb2hsluv(
        static_cast<double>(rgb.r) / 255.0,
        static_cast<double>(rgb.g) / 255.0,
        static_cast<double>(rgb.b) / 255.0,
        &hsl.h, &hsl.s, &hsl.l
    );
    return hsl;
}

sf::Color hslub_to_color(const HSLuvColor& hsl) {
    double r, g, b;
    hsluv2rgb(hsl.h, hsl.s, hsl.l, &r, &g, &b);
    r = std::clamp(r, 0.0, 1.0);
    g = std::clamp(g, 0.0, 1.0);
    b = std::clamp(b, 0.0, 1.0);
    return {
        static_cast<sf::Uint8>(r*255),
        static_cast<sf::Uint8>(g*255),
        static_cast<sf::Uint8>(b*255)
    };
}