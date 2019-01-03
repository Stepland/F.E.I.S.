#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include "screen.h"
#include "EditorState.h"
#include "tinyfiledialogs.h"

int main(int argc, char** argv) {

	// Création de la fenêtre
	sf::RenderWindow window(sf::VideoMode(800, 600), "FEIS");
	sf::RenderWindow & ref_window = window;
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

	Ecran_attente bg;
	std::optional<EditorState> editorState;

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

		ImGui::SFML::Update(window, deltaClock.restart());

		// Dessin du fond
		if (editorState) {
		    if (editorState->showProperties) {
                ImGui::Begin("Properties",&editorState->showProperties);
                ImGui::InputText("Title",&editorState->fumen.songTitle);
                ImGui::InputText("Artist",&editorState->fumen.artist);
                ImGui::InputText("Music Path",&editorState->fumen.musicPath);
                ImGui::InputText("Jacket Path",&editorState->fumen.jacketPath);
                ImGui::End();
		    }
			window.clear(sf::Color(0, 0, 0));
		} else {
			bg.render(window);
		}

		// Dessin de l'interface

		ImGui::BeginMainMenuBar();
		{
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    Fumen f;
                    editorState.emplace(f);
                }
                ImGui::Separator();
				if (ImGui::MenuItem("Open")) {
				    const char* _filepath = tinyfd_openFileDialog("Open File",nullptr,0,nullptr,nullptr,false);
				    if (_filepath != nullptr) {
				        std::string filepath(_filepath);
                        Fumen f;
                        std::optional<std::string> error_message;
                        try {
                            f.loadFromMemon(filepath);
                            editorState.emplace(f);
                        } catch (const std::exception& e) {
                            std::string message(e.what());
                            tinyfd_messageBox("Error",message.c_str(),"ok","error",1);
                        }
                    }
                }
				if (ImGui::BeginMenu("Recent Files")) {
				    for (int i = 1; i<=10; i++) {
				        ImGui::PushID(i);
                        std::ostringstream stringStream;
                        stringStream << i << ". fichier blabla";
                        std::string copyOfStr = stringStream.str();
                        ImGui::MenuItem(copyOfStr.c_str());
                        ImGui::PopID();
				    }
				    ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Save")) {
                    char const * options[1] = {"*.memon"};
                    const char* _filepath(tinyfd_saveFileDialog("Save File",nullptr,1,options,nullptr));
                    if (_filepath != nullptr) {
                        std::string filepath(_filepath);
                        editorState->fumen.saveAsMemon(filepath);
                    }
				}
				ImGui::EndMenu();
			}
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Properties",NULL,false,editorState.has_value())) {
                    editorState.value().showProperties = true;
                }
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