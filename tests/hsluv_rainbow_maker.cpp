#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <cmath>
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics.hpp>

#include "hsluv/hsluv.h"

void imgui_main() {
    static float H_offset = 0.f;
    static float S = 100.f;
    static float L = 100.f;
    static int colors = 16;
    static double R = 1.f;
    static double G = 1.f;
    static double B = 1.f;
    static auto color = sf::Color::White;
    if (ImGui::Begin("HSLuv rainbow maker")) {
        ImGui::SliderFloat("H (offset)", &H_offset, 0, 360);
        ImGui::SliderFloat("S", &S, 0, 100);
        ImGui::SliderFloat("L", &L, 0, 100);
        if (ImGui::InputInt("Colors", &colors)) {
            colors = std::max(0, colors);
        }
        for (int i = 0; i < colors; i++) {
            float hue = std::fmod(H_offset + (static_cast<float>(i) * 360.f) / static_cast<float>(colors), 360.f);
            hsluv2rgb(hue, S, L, &R, &G, &B);
            color.r = static_cast<sf::Uint8>(R * 255.f);
            color.g = static_cast<sf::Uint8>(G * 255.f);
            color.b = static_cast<sf::Uint8>(B * 255.f);
            ImGui::ColorButton("blabla", color);
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
    ImGui::End();
}

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
        imgui_main();
        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

