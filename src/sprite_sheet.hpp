#pragma once

#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <filesystem>
#include <optional>

#include <json.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "utf8_sfml_redefinitions.hpp"

class SpriteSheet {
public:
    SpriteSheet(
        const std::filesystem::path& texture_path,
        std::size_t count,
        std::size_t columns,
        std::size_t rows
    );

    static SpriteSheet load_from_json(
        const nlohmann::json& dict,
        const std::filesystem::path& parent_folder
    );

    sf::Sprite at(std::size_t frame) const;
    std::size_t size() const;

private:
    feis::Texture tex;
    std::size_t count;
    std::size_t columns;
    std::size_t rows;
    sf::Vector2i sprite_size;
};