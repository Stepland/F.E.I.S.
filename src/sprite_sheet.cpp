#include "sprite_sheet.hpp"

#include <stdexcept>

#include "fmt/core.h"

#include "utf8_strings.hpp"

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

/*
    {
        "sprite_sheet": "approach.png",
        "count": 16,
        "columns": 4,
        "rows": 4
    }
*/
SpriteSheet SpriteSheet::load_from_json(
    const nlohmann::json& obj,
    const std::filesystem::path& parent_folder
) {
    auto texture_path = to_path(obj.at("sprite_sheet").get<std::string>());
    if (texture_path.is_relative()) {
        texture_path = parent_folder / texture_path;
    }

    const auto count = obj.at("count").get<std::size_t>();
    const auto columns = obj.at("columns").get<std::size_t>();
    const auto rows = obj.at("rows").get<std::size_t>();

    return SpriteSheet{
        texture_path,
        count,
        columns,
        rows
    };
}

sf::Sprite SpriteSheet::at(std::size_t frame) const {
    if (frame >= count) {
        throw std::out_of_range(fmt::format("frame {} is outside of the SpriteSheet range ({})", frame, count));
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

std::size_t SpriteSheet::size() const {
    return count;
}