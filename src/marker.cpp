#include "marker.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <fmt/core.h>
#include <stdexcept>

#include "utf8_sfml.hpp"
#include "utf8_strings.hpp"

Marker first_available_marker_in(const std::filesystem::path& assets_folder) {
    for (auto& folder : std::filesystem::directory_iterator(assets_folder / "textures" / "markers")) {
        try {
            return Marker{folder};
        } catch (const std::runtime_error&) {}
    }
    throw std::runtime_error("No valid marker found");
}

Marker::Marker(const std::filesystem::path& folder_):
    fps(30),
    folder(folder_)
{
    const auto emplace_back = [&](Marker::texture_vector_type& vec, const std::string& file){
        auto& tex = vec.emplace_back();
        const auto path = folder / file;
        if (not tex.load_from_path(path)) {
            throw std::runtime_error(fmt::format(
                "Unable to load marker {} - failed on image {}",
                path_to_utf8_encoded_string(folder),
                file
            ));
        } else {
            tex.setSmooth(true);
        }
    };

    for (int num = 100; num < 116; num++) {
        emplace_back(poor, fmt::format("h{:03}.png", num));
    }

    for (int num = 200; num < 216; num++) {
        emplace_back(good, fmt::format("h{:03}.png", num));
    }

    for (int num = 300; num < 316; num++) {
        emplace_back(great, fmt::format("h{:03}.png", num));
    }

    for (int num = 400; num < 416; num++) {
        emplace_back(perfect, fmt::format("h{:03}.png", num));
    }

    for (int num = 0; num < 16; num++) {
        emplace_back(approach, fmt::format("ma{:02}.png", num));
    }

    for (int num = 16; num < 24; num++) {
        emplace_back(miss, fmt::format("ma{:02}.png", num));
    }
};

Marker::optional_texture_reference Marker::at(Judgement state, sf::Time offset) {
    const auto frame = static_cast<int>(std::floor(offset.asSeconds() * fps));
    if (frame < 0) {
        const auto approach_frames = static_cast<int>(approach.size());
        const auto index = approach_frames + frame;
        if (index >= 0 and index < approach_frames) {
            return approach.at(index);
        } else {
            return {};
        }
    }

    auto& vec = texture_vector_of(state);
    if (frame < static_cast<int>(vec.size())) {
        return vec.at(frame);
    } else {
        return {};
    }
}

Marker::texture_reference Marker::preview(Judgement state) {
    return texture_vector_of(state).at(2);
}

Marker::texture_vector_type& Marker::texture_vector_of(Judgement state) {
    switch (state) {
        case Judgement::Perfect:
            return perfect;
        case Judgement::Great:
            return great;
        case Judgement::Good:
            return good;
        case Judgement::Poor:
            return poor;
        case Judgement::Miss:
            return miss;
        default:
            throw std::invalid_argument("Unexpected judgement value");
    }
}