#define IMGUI_USER_CONFIG "imconfig-SFML.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>

#include "track.hpp"

void print_mouse_wheel_event(const sf::Event::MouseWheelScrollEvent& event) {
    std::cout << "MouseWheelScrollEvent{";
    if (event.wheel == sf::Mouse::Wheel::VerticalWheel) {
        std::cout << "wheel=VerticalWheel, "; 
    } else {
        std::cout << "wheel=HorizontalWheel, ";
    }
    std::cout << "delta=" << event.delta << ", ";
    std::cout << "x=" << event.x << ", ";
    std::cout << "y=" << event.y << "}";
    std::cout << std::endl;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "scrollwheel");
    ImGui::SFML::Init(window);
    sf::Clock deltaClock;
    while (true) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::MouseWheelScrolled) {
                print_mouse_wheel_event(event.mouseWheelScroll);
            }
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowSize(ImVec2(204, 400), ImGuiCond_Once);
        ImGui::SetNextWindowSizeConstraints(ImVec2(204, 204), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
        if (ImGui::Begin("Hello, world!")) {
            auto cursor_y = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            track("cursor_y", cursor_y);
            track("cursor_screen_y", ImGui::GetCursorScreenPos().y);
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        
        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

