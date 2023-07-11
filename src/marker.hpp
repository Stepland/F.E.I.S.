#pragma once

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <variant>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "sprite_sheet.hpp"
#include "utf8_sfml_redefinitions.hpp"

enum class Judgement {
    Perfect,
    Great,
    Good,
    Poor,
    Miss
};

const static std::unordered_map<Judgement, std::string> judgement_to_name = {
    {Judgement::Perfect, "PERFECT"},
    {Judgement::Great, "GREAT"},
    {Judgement::Good, "GOOD"},
    {Judgement::Poor, "POOR"},
    {Judgement::Miss, "MISS"}
};

const static std::unordered_map<std::string, Judgement> name_to_judgement = {
    {"PERFECT", Judgement::Perfect},
    {"GREAT", Judgement::Great},
    {"GOOD", Judgement::Good},
    {"POOR", Judgement::Poor},
    {"MISS", Judgement::Miss}
};

/*
 * Holds the textures associated with a given marker folder from the assets
 * folder
 */
class Marker {
public:
    virtual std::optional<sf::Sprite> at(Judgement state, sf::Time offset) const = 0;
    virtual sf::Sprite judgement_preview(Judgement state) const = 0;
    virtual sf::Sprite approach_preview() const = 0;
    virtual const std::filesystem::path& get_folder() const = 0;

    virtual ~Marker() = default;
};

class OldMarker : public Marker {
public:
    explicit OldMarker(const std::filesystem::path& folder);

    std::optional<sf::Sprite> at(Judgement state, sf::Time offset) const override;
    sf::Sprite judgement_preview(Judgement state) const override;
    sf::Sprite approach_preview() const;

    const std::filesystem::path& get_folder() const override;
private:
    unsigned int fps = 30;

    using texture_type = feis::Texture;
    using texture_vector_type = std::vector<texture_type>;

    texture_vector_type approach;
    texture_vector_type perfect;
    texture_vector_type great;
    texture_vector_type good;
    texture_vector_type poor;
    texture_vector_type miss;

    const texture_vector_type& texture_vector_of(Judgement state) const;
    
    std::filesystem::path folder;
};

class JujubeMarker : public Marker {
public:
    explicit JujubeMarker(
        const std::size_t fps,
        const SpriteSheet& approach,
        const SpriteSheet& perfect,
        const SpriteSheet& great,
        const SpriteSheet& good,
        const SpriteSheet& poor,
        const SpriteSheet& miss,
        const std::filesystem::path& folder
    );
    static JujubeMarker load_from_folder(const std::filesystem::path& folder);
    
    std::optional<sf::Sprite> at(Judgement state, sf::Time offset) const override;
    sf::Sprite judgement_preview(Judgement state) const override;
    sf::Sprite approach_preview() const;
    
    const std::filesystem::path& get_folder() const override;
private:
    std::size_t fps;

    SpriteSheet approach;
    SpriteSheet perfect;
    SpriteSheet great;
    SpriteSheet good;
    SpriteSheet poor;
    SpriteSheet miss;

    const SpriteSheet& sprite_sheet_of(Judgement state) const;

    std::filesystem::path folder;
};

std::shared_ptr<Marker> load_marker_from(const std::filesystem::path& folder);
std::shared_ptr<Marker> first_available_marker_in(const std::filesystem::path& assets_folder);
