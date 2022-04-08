#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

enum class Judgement {
    Perfect,
    Great,
    Good,
    Early,
    Miss
};

struct MarkerStatePreview {
    Judgement state;
    std::string name;
};

const static std::vector<MarkerStatePreview> marker_state_previews {
    {Judgement::Perfect, "PERFECT"},
    {Judgement::Great, "GREAT"},
    {Judgement::Good, "GOOD"},
    {Judgement::Early, "EARLY / LATE"},
    {Judgement::Miss, "MISS"}
};

using ref_tex = std::reference_wrapper<sf::Texture>;
using opt_ref_tex = std::optional<ref_tex>;

/*
 * Holds the textures associated with a given marker folder from the assets
 * folder
 */
class Marker {
public:
    explicit Marker(const std::filesystem::path& folder);
    opt_ref_tex at(Judgement state, sf::Time offset);
    ref_tex preview(Judgement state);
private:
    unsigned int fps = 30;
    std::vector<sf::Texture> approach;
    std::vector<sf::Texture> perfect;
    std::vector<sf::Texture> great;
    std::vector<sf::Texture> good;
    std::vector<sf::Texture> early;
    std::vector<sf::Texture> miss;

    std::vector<sf::Texture>& texture_vector_of(Judgement state);
};

Marker first_available_marker_from_folder(const std::filesystem::path& assets_folder);
