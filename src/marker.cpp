#include "marker.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <fmt/core.h>
#include <stdexcept>

Marker first_available_marker_in(
    const std::filesystem::path& assets_folder
) {
    for (auto& folder : std::filesystem::directory_iterator(assets_folder / "textures" / "markers")) {
        try {
            return Marker{folder};
        } catch (const std::runtime_error&) {}
    }
    throw std::runtime_error("No valid marker found");
}

Marker::Marker(const std::filesystem::path& folder):
    fps(30)
{
    const auto emplace_back = [&](std::vector<sf::Texture>& vec, const std::string& file){
        auto& tex = vec.emplace_back();
        const auto path = folder / file;
        if (not tex.loadFromFile(path.string())) {
            throw std::runtime_error(fmt::format(
                "Unable to load marker {} - failed on image {}",
                folder.string(),
                file
            ));
        } else {
            tex.setSmooth(true);
        }
    };

    for (int num = 100; num < 116; num++) {
        emplace_back(early, fmt::format("h{:03}.png", num));
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

opt_ref_tex Marker::at(Judgement state, sf::Time offset) {
    const auto frame = static_cast<int>(std::floor(offset.asSeconds() * fps));
    if (frame < 0) {
        const auto index = static_cast<int>(approach.size()) - frame;
        if (index >= 0 and index < approach.size()) {
            return approach.at(index);
        } else {
            return {};
        }
    }

    auto& vec = texture_vector_of(state);
    if (frame < vec.size()) {
        return vec.at(frame);
    } else {
        return {};
    }
}

ref_tex Marker::preview(Judgement state) {
    return texture_vector_of(state).at(2);
}

std::vector<sf::Texture>& Marker::texture_vector_of(Judgement state) {
    switch (state) {
        case Judgement::Perfect:
            return perfect;
        case Judgement::Great:
            return great;
        case Judgement::Good:
            return good;
        case Judgement::Early:
            return early;
        case Judgement::Miss:
            return miss;
    }
}