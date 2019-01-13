//
// Created by Syméon on 17/08/2017.
//

#include "Widgets.h"
#include "Toolbox.h"

Ecran_attente::Ecran_attente() : gris_de_fond(sf::Color(38,38,38)) {

	if(!tex_FEIS_logo.loadFromFile("assets/textures/FEIS_logo.png"))
	{
		throw std::string("Unable to load assets/textures/FEIS_logo.png");
	}
	tex_FEIS_logo.setSmooth(true);
	FEIS_logo.setTexture(tex_FEIS_logo);
	FEIS_logo.setColor(sf::Color(255, 255, 255, 32)); // un huitième opaque

}

void Ecran_attente::render(sf::RenderWindow& window) {
    // effacement de la fenêtre en noir
    window.clear(gris_de_fond);

    // c'est ici qu'on dessine tout
    FEIS_logo.setPosition(sf::Vector2f(static_cast<float>((window.getSize().x-tex_FEIS_logo.getSize().x)/2),
                                       static_cast<float>((window.getSize().y-tex_FEIS_logo.getSize().y)/2)));
    window.draw(FEIS_logo);
}

Playfield::Playfield() {

	marker = Marker();
	if (!button.loadFromFile(button_path,{0,0,192,192})) {
		std::cerr << "Unable to load texture " << button_path;
		throw std::runtime_error("Unable to load texture " + button_path);
	}
	if (!button_pressed.loadFromFile(button_path,{192,0,192,192})) {
		std::cerr << "Unable to load texture " << button_path;
		throw std::runtime_error("Unable to load texture " + button_path);
	}
}

void Playfield::render(sf::RenderWindow &window, EditorState& editorState) {

	ImGui::SetNextWindowSizeConstraints(ImVec2(0,0),ImVec2(FLT_MAX,FLT_MAX),Toolbox::CustomConstraints::ContentSquare);
	ImGui::Begin("Playfield",&editorState.showPlayfield,ImGuiWindowFlags_NoScrollbar);
	{
		float squareSize = ImGui::GetWindowSize().x / 4.f;
		float TitlebarHeight = ImGui::GetWindowSize().y - ImGui::GetWindowSize().x;
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 4; ++x) {
				ImGui::PushID(x+4*y);
				ImGui::SetCursorPos({x*squareSize,TitlebarHeight + y*squareSize});
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0,0,0,0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0,0,1.f,0.1f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0,0,1.f,0.5f));
				ImGui::ImageButton(button,{squareSize,squareSize},0);
				ImGui::PopStyleColor(3);
				ImGui::PopID();
			}
		}
	}
	ImGui::End();
}
