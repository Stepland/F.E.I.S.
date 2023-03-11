#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <unordered_map>
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
    using texture_type = feis::Texture;
    using texture_vector_type = std::vector<texture_type>;
    using texture_reference = std::reference_wrapper<sf::Texture>;
    using optional_texture_reference = std::optional<texture_reference>;
    
    explicit Marker(const std::filesystem::path& folder);
    optional_texture_reference at(Judgement state, sf::Time offset);
    texture_reference preview(Judgement state);

    std::filesystem::path get_folder() {return folder;};
private:
    unsigned int fps = 30;

    texture_vector_type approach;
    texture_vector_type perfect;
    texture_vector_type great;
    texture_vector_type good;
    texture_vector_type poor;
    texture_vector_type miss;

    texture_vector_type& texture_vector_of(Judgement state);
    std::filesystem::path folder;
};

Marker first_available_marker_in(const std::filesystem::path& assets_folder);
