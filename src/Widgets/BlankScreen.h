#ifndef FEIS_BLANKSCREEN_H
#define FEIS_BLANKSCREEN_H

#include <SFML/Graphics.hpp>

class BlankScreen {
public:

    BlankScreen();
    void render(sf::RenderWindow &window);

private:

    sf::Color gris_de_fond;
    sf::Texture tex_FEIS_logo;
    sf::Sprite FEIS_logo;

};

#endif //FEIS_BLANKSCREEN_H
