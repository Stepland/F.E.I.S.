#pragma once

#include <array>
#include <filesystem>
#include <iomanip>
#include <list>
#include <map>
#include <optional>

#include <fmt/core.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>

#include "utf8_sfml_redefinitions.hpp"

/*
Allows storing and querying the textures that make up the tail of a long note

All the *_at() methods purposefully take in an sf::Time and not a frame number
to make the code easier to adapt when I add support for markers that aren't
30 fps

(lol as if that will ever happen, YAGNI)
*/
class LNMarker {
public:
    using texture_type = feis::Texture;
    using texture_reference = std::reference_wrapper<const texture_type>;
    using optional_texture_reference = std::optional<texture_reference>;

    explicit LNMarker(std::filesystem::path folder);

    optional_texture_reference triangle_at(const sf::Time& offset) const;
    optional_texture_reference highlight_at(const sf::Time& offset) const;
    optional_texture_reference outline_at(const sf::Time& offset) const;
    optional_texture_reference background_at(const sf::Time& offset) const;
    optional_texture_reference tail_at(const sf::Time& offset) const;

    bool triangle_cycle_displayed_at(const sf::Time& offset) const;

private:
    std::array<texture_type, 16> triangle_appearance;
    std::array<texture_type, 8> triangle_begin_cycle;
    std::array<texture_type, 16> triangle_cycle;

    // I suppose you just layer the next 3 ?
    std::array<texture_type, 16> square_highlight;
    std::array<texture_type, 16> square_outline;
    std::array<texture_type, 16> square_background;

    std::array<texture_type, 16> tail_cycle;
};

int frame_from_offset(const sf::Time& offset);

template<std::size_t number>
std::array<texture_type, number> load_tex_with_prefix(
    const std::filesystem::path& folder,
    const std::string& prefix,
    const unsigned int first = 0
) {
    std::array<LNMarker::texture_type, number> res;
    for (unsigned int frame = first; frame <= first + number - 1; frame++) {
        auto filename = fmt::format(
            "{prefix}{frame:03}.png",
            fmt::arg("prefix", prefix),
            fmt::arg("frame", frame)
        );
        std::filesystem::path texFile = folder / filename;
        feis::Texture tex;
        if (not tex.load_from_path(texFile)) {
            throw std::runtime_error(fmt::format(
                "Unable to load texture folder {}, failed on texture {}",
                folder.string(),
                filename
            ));
        }
        tex.setSmooth(true);
        res.at(frame - first) = tex;
    }
    return res;
};