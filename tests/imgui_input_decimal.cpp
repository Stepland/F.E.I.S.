#include <cmath>
#include <stdexcept>
#include <sstream>

#include <algorithm>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <libmpdec++/decimal.hh>
#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>

using decimal::Decimal;

bool InputDecimal(const char *label, Decimal* value) {
    auto s = value->format("f");
    if (ImGui::InputText(label, &s, ImGuiInputTextFlags_CharsDecimal)) {
        Decimal new_value;
        try {
            new_value = Decimal{s};
        } catch (const decimal::DecimalException& ex) {
            return false;
        }
        *value = new_value;
        return true;
    }
    return false;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "demo");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    sf::Clock deltaClock;
    Decimal d = 0;
    while (true) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
        }
        ImGui::SFML::Update(window, deltaClock.restart());
        if (ImGui::Begin("Testing InputDecimal")) {
            InputDecimal("My Decimal", &d);
            std::stringstream ss;
            ss << "Actual value : " << d;
            ImGui::TextUnformatted(ss.str().c_str());
        }
        ImGui::End();
        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

