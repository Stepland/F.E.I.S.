//
// Created by Syméon on 17/08/2017.
//

#include "screen.h"

Ecran_attente::Ecran_attente() : gris_de_fond(sf::Color(38,38,38)) {

	if(!tex_FEIS_logo.loadFromFile("textures/FEIS_logo.png"))
	{
		throw std::string("Unable to load textures/FEIS_logo.png");
	}
	FEIS_logo.setTexture(tex_FEIS_logo);
	FEIS_logo.setColor(sf::Color(255, 255, 255, 32)); // un huitième opaque

}

void Ecran_attente::run(sf::RenderWindow& window) {
	// on fait tourner le programme jusqu'à ce que la fenêtre soit fermée
	bool fini = false;
	while (!fini)
	{
		// on inspecte tous les évènements de la fenêtre qui ont été émis depuis la précédente itération
		sf::Event event;
		while (window.pollEvent(event))
		{
			// évènement "fermeture demandée" : on ferme la fenêtre
			if (event.type == sf::Event::Closed) {
				fini = true;
			} else if (event.type == sf::Event::Resized) {
				window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
			}
		}

		// effacement de la fenêtre en noir
		window.clear(gris_de_fond);

		// c'est ici qu'on dessine tout
		FEIS_logo.setPosition(sf::Vector2f((window.getSize().x-tex_FEIS_logo.getSize().x)/2,
		                                   (window.getSize().y-tex_FEIS_logo.getSize().y)/2));
		window.draw(FEIS_logo);

		// fin de la frame courante, affichage de tout ce qu'on a dessiné
		window.display();
	}
}

Ecran_edition::Ecran_edition() : couleur_de_fond(sf::Color(0,0,0)) {

	marker = Marker();

}

void Ecran_edition::run(sf::RenderWindow &window) {

	sf::Clock clock;
	bool fini = false;
	while (!fini)
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) {
				fini = true;
			} else if (event.type == sf::Event::Resized) {
				window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
			}
		}

		window.clear(couleur_de_fond);

		// c'est ici qu'on dessine tout
		sf::Time temps = clock.getElapsedTime();
		int frame = ((int)(30*temps.asSeconds()))%45;
		if (frame < 16) {
			sf::Sprite sprite_marker = marker.getSprite(APPROCHE,frame);
			sprite_marker.setPosition(sf::Vector2f((window.getSize().x-160)/2,
			                                       (window.getSize().y-160)/2));
			window.draw(sprite_marker);
		} else if (frame < 32) {
			sf::Sprite sprite_marker = marker.getSprite(PERFECT,frame-16);
			sprite_marker.setPosition(sf::Vector2f((window.getSize().x-160)/2,
			                                       (window.getSize().y-160)/2));
			window.draw(sprite_marker);
		}


		// fin de la frame courante, affichage de tout ce qu'on a dessiné
		window.display();
	}
}
