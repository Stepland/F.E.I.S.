#pragma once

#include <filesystem>
#include <iomanip>
#include <list>
#include <map>

#include <fmt/core.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>

using opt_tex_ref = std::optional<std::reference_wrapper<const sf::Texture>>;

/*
Allows storing and querying the textures that make up the tail of a long note

All the *_at() methods purposefully take in an sf::Time and not a frame number
to make the code easier to adapt when I add support for markers that aren't
30 fps

(lol as if that will ever happen, YAGNI)
*/
class LNMarker {
public:
    explicit LNMarker(std::filesystem::path folder);

    opt_tex_ref triangle_at(const sf::Time& offset) const;
    opt_tex_ref highlight_at(const sf::Time& offset) const;
    opt_tex_ref outline_at(const sf::Time& offset) const;
    opt_tex_ref background_at(const sf::Time& offset) const;
    opt_tex_ref tail_at(const sf::Time& offset) const;

    bool triangle_cycle_displayed_at(const sf::Time& offset) const;

private:
    std::array<sf::Texture, 16> triangle_appearance;
    std::array<sf::Texture, 8> triangle_begin_cycle;
    std::array<sf::Texture, 16> triangle_cycle;

    // I suppose you just layer the next 3 ?
    std::array<sf::Texture, 16> square_highlight;
    std::array<sf::Texture, 16> square_outline;
    std::array<sf::Texture, 16> square_background;

    std::array<sf::Texture, 16> tail_cycle;
};

int frame_from_offset(const sf::Time& offset);

template<std::size_t number, unsigned int first = 0>
std::array<sf::Texture, number> load_tex_with_prefix(
    const std::filesystem::path& folder,
    const std::string& prefix
) {
    std::array<sf::Texture, number> res;
    for (unsigned int frame = first; frame <= first + number - 1; frame++) {
        auto filename = fmt::format(
            "{prefix}{frame:03}.png",
            fmt::arg("prefix", prefix),
            fmt::arg("frame", frame)
        );
        std::filesystem::path texFile = folder / filename;
        sf::Texture tex;
        if (!tex.loadFromFile(texFile.string())) {
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