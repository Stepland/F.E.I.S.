#ifndef FEIS_LNMARKER_H
#define FEIS_LNMARKER_H

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <filesystem>
#include <iomanip>
#include <list>
#include <map>

/*
 * Stores every rotated variant of the long note marker
 * This approach is absolutely terrible, I should just dig a little bit into the
 * internals of Dear ImGui and pass in custom UVs when I want to rotate a
 * texture but I don't feel confident enough rn
 */
class LNMarker {
public:
    explicit LNMarker(std::filesystem::path folder = "assets/textures/long");

    std::optional<std::reference_wrapper<sf::Texture>> getTriangleTexture(float seconds);

    std::optional<std::reference_wrapper<sf::Texture>>
    getSquareHighlightTexture(float seconds);
    std::optional<std::reference_wrapper<sf::Texture>> getSquareOutlineTexture(float seconds);
    std::optional<std::reference_wrapper<sf::Texture>>
    getSquareBackgroundTexture(float seconds);

    std::optional<std::reference_wrapper<sf::Texture>> getTailTexture(float seconds);

private:
    std::array<sf::Texture, 16> triangle_appearance;
    std::array<sf::Texture, 8> triangle_begin_cycle;
    std::array<sf::Texture, 16> triangle_cycle;

    // I suppose you just layer the next 3 ?
    std::array<sf::Texture, 16> square_highlight;
    std::array<sf::Texture, 16> square_outline;
    std::array<sf::Texture, 16> square_background;

    std::array<sf::Texture, 16> tail_cycle;

    template<int number, int first = 0>
    std::array<sf::Texture, number> load_tex_with_prefix(
        const std::filesystem::path& folder,
        const std::string& prefix,
        int left_padding = 3,
        const std::string& extension = ".png") {
        std::array<sf::Texture, number> res;
        for (int frame = first; frame <= first + number - 1; frame++) {
            std::stringstream filename;
            filename << prefix << std::setfill('0') << std::setw(left_padding)
                     << frame << extension;
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
    }

    template<int number>
    void setRepeated(std::array<sf::Texture, number> tex, bool repeat) {
        for (int i = 0; i < number; ++i) {
            tex.at(i).setRepeated(repeat);
        }
    }
};

#endif  // FEIS_LNMARKER_H
