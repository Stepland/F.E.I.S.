//
// Created by Syméon on 17/08/2017.
//

#include "Widgets.h"
#include "Toolbox.h"

Widgets::Ecran_attente::Ecran_attente() : gris_de_fond(sf::Color(38,38,38)) {

	if(!tex_FEIS_logo.loadFromFile("assets/textures/FEIS_logo.png"))
	{
		throw std::string("Unable to load assets/textures/FEIS_logo.png");
	}
	tex_FEIS_logo.setSmooth(true);
	FEIS_logo.setTexture(tex_FEIS_logo);
	FEIS_logo.setColor(sf::Color(255, 255, 255, 32)); // un huitième opaque

}

void Widgets::Ecran_attente::render(sf::RenderWindow& window) {
    // effacement de la fenêtre en noir
    window.clear(gris_de_fond);

    // c'est ici qu'on dessine tout
    FEIS_logo.setPosition(sf::Vector2f(static_cast<float>((window.getSize().x-tex_FEIS_logo.getSize().x)/2),
                                       static_cast<float>((window.getSize().y-tex_FEIS_logo.getSize().y)/2)));
    window.draw(FEIS_logo);
}

Widgets::Playfield::Playfield() {
	if (!button.loadFromFile(button_path,{0,0,192,192})) {
		std::cerr << "Unable to load texture " << button_path;
		throw std::runtime_error("Unable to load texture " + button_path);
	}
	if (!button_pressed.loadFromFile(button_path,{192,0,192,192})) {
		std::cerr << "Unable to load texture " << button_path;
		throw std::runtime_error("Unable to load texture " + button_path);
	}
}
