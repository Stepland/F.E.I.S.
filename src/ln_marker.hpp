#pragma once

#include <filesystem>
#include <iomanip>
#include <list>
#include <map>

#include <fmt/core.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

using opt_tex_ref = std::optional<std::reference_wrapper<sf::Texture>>;

/*
 * Stores every rotated variant of the long note marker
 * This approach is absolutely terrible, I should just dig a little bit into the
 * internals of Dear ImGui and pass in custom UVs when I want to rotate a
 * texture but I don't feel confident enough rn
 */
class LNMarker {
public:
    explicit LNMarker(std::filesystem::path folder);

    opt_tex_ref triangle_at(int frame);
    opt_tex_ref highlight_at(int frame);
    opt_tex_ref outline_at(int frame);
    opt_tex_ref background_at(int frame);
    opt_tex_ref tail_at(int frame);

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

template<std::size_t number, unsigned int first = 0>
std::array<sf::Texture, number> load_tex_with_prefix(
    const std::filesystem::path& folder,
    const std::string& prefix,
) {
    std::array<sf::Texture, number> res;
    for (unsigned int frame = first; frame <= first + number - 1; frame++) {
        auto filename = fmt::format(
            "{prefix}{frame:03}.png",
            fmt::arg("prefix", prefix),
            fmt::arg("frame", frame)
        )
        std::stringstream filename;
        filename << prefix << std::setfill('0') << std::setw(3)
                    << frame << ".png";
        std::filesystem::path texFile = folder / filename.str();
        sf::Texture tex;
        if (!tex.loadFromFile(texFile.string())) {
            std::stringstream err;
            err << "Unable to load texture folder " << folder
                << "\nfailed on texture " << filename.str();
            throw std::runtime_error(err.str());
        }
        tex.setSmooth(true);
        res.at(frame - first) = tex;
    }
    return res;
};