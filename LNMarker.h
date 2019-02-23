//
// Created by Syméon on 09/02/2019.
//

#ifndef FEIS_LNMARKER_H
#define FEIS_LNMARKER_H

#include <filesystem>
#include <list>
#include <map>
#include <iomanip>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

/*
 * This approach at displaying rotated textures is absolutely terrible, I should just dig a little bit into the internals
 * of Dear ImGui and pass in custom vertices and UVs when I want to rotate a texture but I don't feel confident enough rn
 */
class LNMarker {

public:
    explicit LNMarker(std::filesystem::path folder="assets/textures/long");

    std::optional<std::reference_wrapper<sf::Texture>> getTriangleTexture(float seconds, int tail_pos);

    std::optional<std::reference_wrapper<sf::Texture>> getSquareHighlightTexture (float seconds, int tail_pos);
    std::optional<std::reference_wrapper<sf::Texture>> getSquareOutlineTexture   (float seconds, int tail_pos);
    std::optional<std::reference_wrapper<sf::Texture>> getSquareBackgroundTexture(float seconds, int tail_pos);

    std::optional<std::reference_wrapper<sf::Texture>> getTailTexture(float seconds, int tail_pos);

private:

    std::array<sf::Texture,16> triangle_appearance;
    std::array<sf::Texture,8>  triangle_begin_cycle;
    std::array<sf::Texture,16> triangle_cycle;

    // I suppose you just layer the next 3 ?
    std::array<sf::Texture,16> square_highlight;
    std::array<sf::Texture,16> square_outline;
    std::array<sf::Texture,16> square_background;

    std::array<sf::Texture,16> tail_cycle;

    std::array<std::array<sf::Texture,16>,4> triangle_appearance_rotated;
    std::array<std::array<sf::Texture, 8>,4> triangle_begin_cycle_rotated;
    std::array<std::array<sf::Texture,16>,4> triangle_cycle_rotated;

    std::array<std::array<sf::Texture,16>,4> square_highlight_rotated;
    std::array<std::array<sf::Texture,16>,4> square_outline_rotated;
    std::array<std::array<sf::Texture,16>,4> square_background_rotated;

    std::array<std::array<sf::Texture,16>,4> tail_cycle_rotated;

    template<int number, int first=0>
    std::array<sf::Texture,number> load_tex_with_prefix(
            const std::filesystem::path &folder,
            const std::string &prefix,
            int left_padding = 3,
            const std::string &extension = ".png") {

        std::array<sf::Texture,number> res;
        for (int frame = first; frame <= first+number-1; frame++) {
            std::stringstream filename;
            filename << prefix << std::setfill('0') << std::setw(left_padding) << frame << extension;
            std::filesystem::path texFile = folder / filename.str();
            sf::Texture tex;
            if (!tex.loadFromFile(texFile.string())) {
                std::stringstream err;
                err << "Unable to load texture folder " << folder << "\nfailed on texture " << filename.str();
                throw std::runtime_error(err.str());
            }
            tex.setSmooth(true);
            res.at(frame-first) = tex;
        }
        return res;

    }
    template<int T>
    void load_rotated_variants(
            std::array<sf::Texture,T>& textures,
            std::array<std::array<sf::Texture,T>,4>& rotated_variants
                    ) {
        for (int rotation = 0; rotation < 4; rotation++) {
            for (auto i = 0; i < textures.size(); ++i) {

                sf::RenderTexture texture_1;
                if (!texture_1.create(160,160)) {
                    throw std::runtime_error("Error creating RenderTexture");
                }
                texture_1.setSmooth(true);

                sf::RenderTexture texture_2;
                if (!texture_2.create(160,160)) {
                    throw std::runtime_error("Error creating RenderTexture");
                }
                texture_2.setSmooth(true);

                auto s = sf::Sprite(textures.at(i));
                s.setOrigin(80.f,80.f);
                s.setPosition(80.f,80.f);
                s.rotate(90.0f*rotation);
                texture_1.draw(s);

                auto s2 = sf::Sprite(texture_1.getTexture());
                texture_2.draw(s2);

                rotated_variants.at(rotation).at(i) = texture_2.getTexture();
                rotated_variants.at(rotation).at(i).setSmooth(true);
                rotated_variants.at(rotation).at(i).setRepeated(true);

            }
        }
    }

};


#endif //FEIS_LNMARKER_H
