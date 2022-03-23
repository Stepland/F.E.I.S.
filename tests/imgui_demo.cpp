#define IMGUI_USER_CONFIG "imconfig-SFML.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics.hpp>

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "demo");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    sf::Clock deltaClock;
    while (true) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
        }
        ImGui::SFML::Update(window, deltaClock.restart());
        ImGui::ShowDemoWindow();
        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

