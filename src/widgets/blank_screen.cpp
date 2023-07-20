#include "blank_screen.hpp"

#include <SFML/System/Vector2.hpp>
#include <string>

#include "toolbox.hpp"
#include "utf8_strings.hpp"

BlankScreen::BlankScreen(std::filesystem::path assets) : background_grey(sf::Color(38, 38, 38)) {
    if (!tex_FEIS_logo.load_from_path(assets / "textures" / "FEIS_logo.png")) {
        throw std::string("Unable to load assets/textures/FEIS_logo.png");
    }
    tex_FEIS_logo.setSmooth(true);
    FEIS_logo.setTexture(tex_FEIS_logo);
    FEIS_logo.setColor(sf::Color(255, 255, 255, 32));
}

void BlankScreen::render(sf::RenderWindow& window) {
    window.clear(background_grey);
    Toolbox::center(FEIS_logo);
    FEIS_logo.setPosition(sf::Vector2f{window.getSize()} / 2.0f);
    window.draw(FEIS_logo);
}