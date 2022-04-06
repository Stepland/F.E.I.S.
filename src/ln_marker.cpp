#include "ln_marker.hpp"

#include <cmath>

LNMarker::LNMarker(std::filesystem::path folder) :
    triangle_appearance(load_tex_with_prefix<16, 400>(folder, "LN0001_M")),
    triangle_begin_cycle(load_tex_with_prefix<8, 600>(folder, "LN0001_M")),
    triangle_cycle(load_tex_with_prefix<16, 500>(folder, "LN0001_M")),
    square_highlight(load_tex_with_prefix<16, 300>(folder, "LN0001_M")),
    square_outline(load_tex_with_prefix<16, 100>(folder, "LN0001_M")),
    square_background(load_tex_with_prefix<16, 200>(folder, "LN0001_M")),
    tail_cycle(load_tex_with_prefix<16, 0>(folder, "LN0001_M"))
{   
    for (tex& : tail_cycle) {
        tex.setRepeated(true);
    }
}

opt_tex_ref LNMarker::triangle_at(int frame) {
    if (frame >= -16 and frame <= -1) {
        // approach phase
        return triangle_appearance.at(16 + frame);
    } else if (frame >= 0) {
        if (frame <= 7) {
            return triangle_begin_cycle.at(frame);
        } else {
            return triangle_cycle.at((frame - 8) % 16);
        }
    } else {
        return {};
    }
}

opt_tex_ref LNMarker::tail_at(int frame) {
    if (frame >= -16) {
        return tail_cycle.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

opt_tex_ref LNMarker::highlight_at(int frame) {
    if (frame >= 0) {
        return square_highlight.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

opt_tex_ref LNMarker::outline_at(int frame) {
    if (frame >= -16) {
        return square_outline.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}

opt_tex_ref LNMarker::background_at(int frame) {
    if (frame >= -16) {
        return square_background.at((16 + (frame % 16)) % 16);
    } else {
        return {};
    }
}