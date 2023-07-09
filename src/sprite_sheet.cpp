#include "sprite_sheet.hpp"

#include <stdexcept>

#include "fmt/core.h"

SpriteSheet::SpriteSheet(
    const std::filesystem::path& texture_path,
    std::size_t count,
    std::size_t columns,
    std::size_t rows
) :
    count(count),
    columns(columns),
    rows(rows)
{
    if (not tex.loadFromFile(texture_path)) {
        throw std::runtime_error(
            "Cannot open file "
            +texture_path.string()
        );
    }
    tex.setSmooth(true);

    auto tex_size = tex.getSize();
    if (tex_size.x % columns != 0) {
        throw std::invalid_argument(fmt::format(
            "{0} is used as a sprite sheet and is expected to be evenly "
            "cut into {1} column{2} according to the metadata, but the image's "
            "width is not divisble by {1}",
            texture_path.string(),
            columns,
            columns == 1 ? "" : "s"
        ));
    }
    if (tex_size.y % rows != 0) {
        throw std::invalid_argument(fmt::format(
            "{0} is used as a sprite sheet and is expected to be evenly "
            "cut into {1} row{2} according to the metadata, but the image's "
            "height is not divisble by {1}",
            texture_path.string(),
            rows,
            rows == 1 ? "" : "s"
        ));
    }

    sprite_size = {
        static_cast<int>(tex_size.x / columns),
        static_cast<int>(tex_size.y / rows)
    };

    if (count > columns * rows) {
        throw std::invalid_argument(fmt::format(
            "Metadata for sprite sheet {}  indicates that it holds {} sprites "
            "but it can only hold a maximum of {} according to the 'columns' "
            "and 'rows' fields",
            texture_path.string(),
            count,
            columns * rows
        ));
    }      
}


SpriteSheet SpriteSheet::load_from_json(
    const nlohmann::json& dict,
    const std::filesystem::path& parent_folder
) {
    
}

std::optional<sf::Sprite> SpriteSheet::at(std::size_t frame) const {
    if (frame >= count) {
        return {};
    }

    sf::Sprite sprite{tex};
    sf::IntRect rect{
        sf::Vector2i{
            static_cast<int>(frame % columns) * sprite_size.x,
            static_cast<int>(frame / columns) * sprite_size.y
        },
        sprite_size
    };
    sprite.setTextureRect(rect);
    return sprite;
}