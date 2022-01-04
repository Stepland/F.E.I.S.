#ifndef FEIS_BLANKSCREEN_H
#define FEIS_BLANKSCREEN_H

#include <SFML/Graphics.hpp>

#include <filesystem>

class BlankScreen {
public:
    BlankScreen(std::filesystem::path assets);
    void render(sf::RenderWindow& window);

private:
    sf::Color gris_de_fond;
    sf::Texture tex_FEIS_logo;
    sf::Sprite FEIS_logo;
};

#endif  // FEIS_BLANKSCREEN_H
