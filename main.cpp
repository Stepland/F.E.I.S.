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

    ImGui::SFML::Init(window, false);

    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->Clear();
    IO.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans-Medium.ttf", 16.f);
    ImGui::SFML::UpdateFontTexture();


	Ecran_attente bg;
	std::optional<EditorState> editorState;

	sf::Clock deltaClock;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);

			switch (event.type) {
			    case sf::Event::Closed:
                    window.close();
			        break;
			    case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
			        break;
			    case sf::Event::KeyPressed:
			        if (event.key.code == sf::Keyboard::Space) {
			            if (not ImGui::IsAnyWindowFocused()) {
                            if (editorState and editorState->music) {
                                switch (editorState->music->getStatus()) {
                                    case sf::Music::Stopped:
                                    case sf::Music::Paused:
                                        editorState->music->play();
                                        break;
                                    case sf::Music::Playing:
                                        editorState->music->pause();
                                        break;
                                }
                            }
			            }
			        }
			        break;
			}
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		// Dessin du fond
		if (editorState) {
		    if (editorState->showProperties) {
		        editorState->displayProperties();
		    }
		    if (editorState->showStatus) {
		        editorState->displayStatus();
		    }
            if (editorState->showPlaybackStatus) {
                editorState->displayPlaybackStatus();
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
                    const char* _filepath = tinyfd_saveFileDialog("New File",nullptr,0,nullptr,nullptr);
                    if (_filepath != nullptr) {
                        std::filesystem::path filepath(_filepath);
                        try {
                            Fumen f(filepath);
                            f.autoSaveAsMemon();
                            editorState.emplace(f);
                        } catch (const std::exception& e) {
                            tinyfd_messageBox("Error",e.what(),"ok","error",1);
                        }
                    }
                }
                ImGui::Separator();
				if (ImGui::MenuItem("Open")) {
				    const char* _filepath = tinyfd_openFileDialog("Open File",nullptr,0,nullptr,nullptr,false);
				    if (_filepath != nullptr) {
                        std::filesystem::path filepath(_filepath);
                        try {
                            Fumen f(filepath);
                            f.autoLoadFromMemon();
                            editorState.emplace(f);
                        } catch (const std::exception& e) {
                            tinyfd_messageBox("Error",e.what(),"ok","error",1);
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
                    try {
                        editorState->fumen.autoSaveAsMemon();
                    } catch (const std::exception& e) {
                        tinyfd_messageBox("Error",e.what(),"ok","error",1);
                    }
				}
				if (ImGui::MenuItem("Save As")) {
                    char const * options[1] = {"*.memon"};
                    const char* _filepath(tinyfd_saveFileDialog("Save File",nullptr,1,options,nullptr));
                    if (_filepath != nullptr) {
                        std::filesystem::path filepath(_filepath);
                        try {
                            editorState->fumen.saveAsMemon(filepath);
                        } catch (const std::exception& e) {
                            tinyfd_messageBox("Error",e.what(),"ok","error",1);
                        }
                    }
				}
				ImGui::EndMenu();
			}
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Properties",nullptr,false,editorState.has_value())) {
                    editorState->showProperties = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View",editorState.has_value())) {
                if (ImGui::MenuItem("Editor Status",nullptr,editorState->showStatus)) {
                    editorState->showStatus = not editorState->showStatus;
                }
                if (ImGui::MenuItem("Playback Status",nullptr,editorState->showPlaybackStatus)) {
                    editorState->showStatus = not editorState->showPlaybackStatus;
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