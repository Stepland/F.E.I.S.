#pragma once

#include <optional>
#include <string>
#include <variant>

#include "better_song.hpp"
#include "special_numeric_types.hpp"

struct PreviewLoopInGui {
    Decimal start;
    Decimal duration;
};

struct MetadataInGui {
    explicit MetadataInGui(const better::Metadata& metadata);
    std::string title;
    std::string artist;
    std::string audio;
    std::string jacket;
    PreviewLoopInGui preview_loop;
    std::string preview_file;
    bool use_preview_file = false;
};