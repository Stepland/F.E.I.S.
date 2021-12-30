#include <cmath>

#include "ln_marker.hpp"

LNMarker::LNMarker(std::filesystem::path folder) {

    triangle_appearance     = load_tex_with_prefix<16,400>(folder,"LN0001_M");
    triangle_begin_cycle    = load_tex_with_prefix< 8,600>(folder,"LN0001_M");
    triangle_cycle          = load_tex_with_prefix<16,500>(folder,"LN0001_M");

    square_highlight        = load_tex_with_prefix<16,300>(folder,"LN0001_M");
    square_outline          = load_tex_with_prefix<16,100>(folder,"LN0001_M");
    square_background       = load_tex_with_prefix<16,200>(folder,"LN0001_M");

    tail_cycle              = load_tex_with_prefix<16,  0>(folder,"LN0001_M");
    setRepeated<16>(tail_cycle,true);

}



std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getTriangleTexture(float seconds) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));

    if (frame >= -16 and frame <= -1) {
        // approach phase
        return triangle_appearance.at(static_cast<unsigned long long int>(16 + frame));
    } else if (frame >= 0) {
        if (frame <= 7) {
            return triangle_begin_cycle.at(static_cast<unsigned long long int>(frame));
        } else {
            return triangle_cycle.at(static_cast<unsigned long long int>((frame - 8) % 16));
        }
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getTailTexture(float seconds) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= -16) {
        return tail_cycle.at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getSquareHighlightTexture(float seconds) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= 0) {
        return square_highlight.at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getSquareOutlineTexture(float seconds) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= -16) {
        return square_outline.at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getSquareBackgroundTexture(float seconds) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= -16) {
        return square_background.at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}
