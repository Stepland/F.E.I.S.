#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>

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

    std::filesystem::path get_folder() {return folder;};
private:
    unsigned int fps = 30;
    std::vector<sf::Texture> approach;
    std::vector<sf::Texture> perfect;
    std::vector<sf::Texture> great;
    std::vector<sf::Texture> good;
    std::vector<sf::Texture> poor;
    std::vector<sf::Texture> miss;

    std::vector<sf::Texture>& texture_vector_of(Judgement state);
    std::filesystem::path folder;
};

Marker first_available_marker_in(const std::filesystem::path& assets_folder);
