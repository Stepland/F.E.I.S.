#include "blank_screen.hpp"

#include <string>

#include "utf8_strings.hpp"

BlankScreen::BlankScreen(std::filesystem::path assets) : gris_de_fond(sf::Color(38, 38, 38)) {
    if (!tex_FEIS_logo.loadFromFile(to_sfml_string(assets / "textures" / "FEIS_logo.png"))) {
        throw std::string("Unable to load assets/textures/FEIS_logo.png");
    }
    tex_FEIS_logo.setSmooth(true);
    FEIS_logo.setTexture(tex_FEIS_logo);
    FEIS_logo.setColor(sf::Color(255, 255, 255, 32));  // un huitième opaque
}

void BlankScreen::render(sf::RenderWindow& window) {
    // effacement de la fenêtre en noir
    window.clear(gris_de_fond);

    // c'est ici qu'on dessine tout
    FEIS_logo.setPosition(sf::Vector2f(
        static_cast<float>((window.getSize().x - tex_FEIS_logo.getSize().x) / 2),
        static_cast<float>((window.getSize().y - tex_FEIS_logo.getSize().y) / 2)));
    window.draw(FEIS_logo);
}