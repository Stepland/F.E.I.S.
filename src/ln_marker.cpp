#include "ln_marker.hpp"

#include <cmath>

LNMarker::LNMarker(std::filesystem::path folder) :
    triangle_appearance(load_tex_with_prefix<16>(folder, "LN0001_M", 400)),
    triangle_begin_cycle(load_tex_with_prefix<8>(folder, "LN0001_M", 600)),
    triangle_cycle(load_tex_with_prefix<16>(folder, "LN0001_M", 500)),
    square_highlight(load_tex_with_prefix<16>(folder, "LN0001_M", 300)),
    square_outline(load_tex_with_prefix<16>(folder, "LN0001_M", 100)),
    square_background(load_tex_with_prefix<16>(folder, "LN0001_M", 200)),
    tail_cycle(load_tex_with_prefix<16>(folder, "LN0001_M",0))
{   
    for (auto& tex : tail_cycle) {
        tex.setRepeated(true);
    }
}

LNMarker::optional_texture_reference LNMarker::triangle_at(const sf::Time& offset) const {
    const auto frame = frame_from_offset(offset);
    if (frame < -16) {
        return {};
    } else if (frame < 0) {
        return triangle_appearance.at(16 + frame);
    } else if (frame < 8) {
        return triangle_begin_cycle.at(frame);
    } else {
        return triangle_cycle.at((frame - 8) % 16);
    }
}

bool LNMarker::triangle_cycle_displayed_at(const sf::Time& offset) const {
    const auto frame = frame_from_offset(offset);
    return frame >= 8;
}

LNMarker::optional_texture_reference LNMarker::tail_at(const sf::Time& offset) const {
    const auto frame = frame_from_offset(offset);
    if (frame >= -16) {
        return tail_cycle.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

LNMarker::optional_texture_reference LNMarker::highlight_at(const sf::Time& offset) const {
    const auto frame = frame_from_offset(offset);
    if (frame >= 0) {
        return square_highlight.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

LNMarker::optional_texture_reference LNMarker::outline_at(const sf::Time& offset) const {
    const auto frame = frame_from_offset(offset);
    if (frame >= -16) {
        return square_outline.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

LNMarker::optional_texture_reference LNMarker::background_at(const sf::Time& offset) const {
    const auto frame = frame_from_offset(offset);
    if (frame >= -16) {
        return square_background.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

int frame_from_offset(const sf::Time& offset) {
    return static_cast<int>(std::floor(offset.asSeconds() * 30.f));
}