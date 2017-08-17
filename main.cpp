#include <SFML/Graphics.hpp>
#include "screen.h"

int main(int argc, char** argv) {

	// Création de la fenêtre
	sf::RenderWindow window(sf::VideoMode(800, 600), "FEIS");
	sf::RenderWindow & ref_window = window;
	window.setVerticalSyncEnabled(true);
	Ecran_attente Idle = Ecran_attente();
	Idle.run(ref_window);
	Ecran_edition Edit = Ecran_edition();
	Edit.run(ref_window);
	return 0;

}