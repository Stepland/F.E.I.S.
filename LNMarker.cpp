#include <cmath>

//
// Created by Syméon on 09/02/2019.
//

#include "LNMarker.h"

LNMarker::LNMarker(std::filesystem::path folder) {

    triangle_appearance     = load_tex_with_prefix<16,400>(folder,"LN0001_M");
    triangle_begin_cycle    = load_tex_with_prefix< 8,600>(folder,"LN0001_M");
    triangle_cycle          = load_tex_with_prefix<16,500>(folder,"LN0001_M");

    square_highlight        = load_tex_with_prefix<16,300>(folder,"LN0001_M");
    square_outline          = load_tex_with_prefix<16,100>(folder,"LN0001_M");
    square_background       = load_tex_with_prefix<16,200>(folder,"LN0001_M");

    tail_cycle              = load_tex_with_prefix<16,  0>(folder,"LN0001_M");

    load_rotated_variants<16>(triangle_appearance,triangle_appearance_rotated);
    load_rotated_variants< 8>(triangle_begin_cycle,triangle_begin_cycle_rotated);
    load_rotated_variants<16>(triangle_cycle,triangle_cycle_rotated);

    load_rotated_variants<16>(square_highlight,square_highlight_rotated);
    load_rotated_variants<16>(square_outline,square_outline_rotated);
    load_rotated_variants<16>(square_background,square_background_rotated);

    load_rotated_variants<16>(tail_cycle,tail_cycle_rotated);
}



std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getTriangleTexture(float seconds, int tail_pos) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));

    if (frame >= -16 and frame <= -1) {
        // approach phase
        return triangle_appearance_rotated.at(tail_pos % 4).at(static_cast<unsigned long long int>(16 + frame));
    } else if (frame >= 0) {
        if (frame <= 7) {
            return triangle_begin_cycle_rotated.at(tail_pos % 4).at(static_cast<unsigned long long int>(frame));
        } else {
            return triangle_cycle_rotated.at(tail_pos % 4).at(static_cast<unsigned long long int>((frame - 8) % 16));
        }
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getTailTexture(float seconds, int tail_pos) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= -16) {
        return tail_cycle_rotated.at(tail_pos%4).at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getSquareHighlightTexture(float seconds, int tail_pos) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= 0) {
        return square_highlight_rotated.at(tail_pos%4).at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getSquareOutlineTexture(float seconds, int tail_pos) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= -16) {
        return square_outline_rotated.at(tail_pos%4).at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}

std::optional<std::reference_wrapper<sf::Texture>> LNMarker::getSquareBackgroundTexture(float seconds, int tail_pos) {
    auto frame = static_cast<long long int>(std::floor(seconds * 30.f));
    if (frame >= -16) {
        return square_background_rotated.at(tail_pos%4).at(static_cast<unsigned long long int>((16 + (frame % 16)) % 16));
    } else {
        return {};
    }
}
