#include "marker.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <exception>
#include <fmt/core.h>
#include <memory>
#include <stdexcept>

#include "sprite_sheet.hpp"
#include "utf8_sfml.hpp"
#include "utf8_strings.hpp"
#include "variant_visitor.hpp"

OldMarker::OldMarker(const std::filesystem::path& folder_):
    fps(30),
    folder(folder_)
{
    const auto emplace_back = [&](OldMarker::texture_vector_type& vec, const std::string& file){
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

std::optional<sf::Sprite> OldMarker::at(Judgement state, sf::Time offset) const {
    const auto frame = static_cast<int>(std::floor(offset.asSeconds() * fps));
    if (frame < 0) {
        const auto approach_frames = static_cast<int>(approach.size());
        const auto index = approach_frames + frame;
        if (index >= 0 and index < approach_frames) {
            return sf::Sprite{approach.at(index)};
        } else {
            return {};
        }
    }

    auto& vec = texture_vector_of(state);
    if (frame < static_cast<int>(vec.size())) {
        return sf::Sprite{vec.at(frame)};
    } else {
        return {};
    }
}

sf::Sprite OldMarker::judgement_preview(Judgement state) const {
    return sf::Sprite{texture_vector_of(state).at(0)};
}

sf::Sprite OldMarker::approach_preview() const {
    return sf::Sprite{approach.back()};
}

const std::filesystem::path& OldMarker::get_folder() const {
    return folder;
}

const OldMarker::texture_vector_type& OldMarker::texture_vector_of(Judgement state) const {
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

JujubeMarker::JujubeMarker(
    const std::size_t _fps,
    const SpriteSheet& _approach,
    const SpriteSheet& _perfect,
    const SpriteSheet& _great,
    const SpriteSheet& _good,
    const SpriteSheet& _poor,
    const SpriteSheet& _miss,
    const std::filesystem::path& _folder
) :
    fps(_fps),
    approach(_approach),
    perfect(_perfect),
    great(_great),
    good(_good),
    poor(_poor),
    miss(_miss),
    folder(_folder)
{}


JujubeMarker JujubeMarker::load_from_folder(const std::filesystem::path& folder) {
    nowide::ifstream f{path_to_utf8_encoded_string(folder / "marker.json")};
    const auto j = nlohmann::json::parse(f);
    return JujubeMarker{
        j.at("fps").get<std::size_t>(),
        SpriteSheet::load_from_json(j.at("approach"), folder),
        SpriteSheet::load_from_json(j.at("perfect"), folder),
        SpriteSheet::load_from_json(j.at("great"), folder),
        SpriteSheet::load_from_json(j.at("good"), folder),
        SpriteSheet::load_from_json(j.at("poor"), folder),
        SpriteSheet::load_from_json(j.at("miss"), folder),
        folder
    };
}

std::optional<sf::Sprite> JujubeMarker::at(Judgement state, sf::Time offset) const {
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

    auto& sheet = sprite_sheet_of(state);
    if (frame < static_cast<int>(sheet.size())) {
        return sheet.at(frame);
    } else {
        return {};
    }
}

sf::Sprite JujubeMarker::judgement_preview(Judgement state) const {
    return sprite_sheet_of(state).at(0);
}

sf::Sprite JujubeMarker::approach_preview() const {
    return approach.at(approach.size() - 1);
}

const std::filesystem::path& JujubeMarker::get_folder() const {
    return folder;
}

const SpriteSheet& JujubeMarker::sprite_sheet_of(Judgement state) const {
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

std::shared_ptr<Marker> load_marker_from(const std::filesystem::path& folder) {
    try {
        return std::make_shared<OldMarker>(folder);
    } catch (const std::exception&) {}
    
    return std::make_shared<JujubeMarker>(std::move(JujubeMarker::load_from_folder(folder)));
}

std::shared_ptr<Marker> first_available_marker_in(const std::filesystem::path& markers_folder) {
    for (auto& folder : std::filesystem::directory_iterator(markers_folder)) {
        try {
            return load_marker_from(folder);
        } catch (const std::exception&) {}
    }
    throw std::runtime_error("No valid marker found");
}