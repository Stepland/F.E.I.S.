#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>
#include "utf8_sfml_redefinitions.hpp"

class BlankScreen {
public:
    BlankScreen(std::filesystem::path assets);
    void render(sf::RenderWindow& window);

private:
    sf::Color background_grey;
    feis::Texture tex_FEIS_logo;
    sf::Sprite FEIS_logo;
};
