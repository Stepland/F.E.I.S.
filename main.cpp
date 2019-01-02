#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "screen.h"
#include "EditorState.h"

int main(int argc, char** argv) {

	// Création de la fenêtre
	sf::RenderWindow window(sf::VideoMode(800, 600), "FEIS");
	sf::RenderWindow & ref_window = window;
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

	Ecran_attente bg;
	std::optional<EditorState> editorState = {};

	sf::Clock deltaClock;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed) {
				window.close();
			} else if (event.type == sf::Event::Resized) {
			    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
			};
		}

		// Dessin du fond
		if (editorState) {
			NULL;
		} else {
			bg.render(window);
		}

		// Dessin de l'interface
        ImGui::SFML::Update(window, deltaClock.restart());

		ImGui::BeginMainMenuBar();
		{
            if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open", "Ctrl+O")) {
					std::cout << "Open clicked" << std::endl;
				};
				ImGui::EndMenu();
			}
        }
        ImGui::EndMainMenuBar();

        ImGui::SFML::Render(window);
		window.display();
	}

    ImGui::SFML::Shutdown();
    return 0;
}