#define IMGUI_USER_CONFIG "imconfig-SFML.h"

#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>

void print_mouse_button_event(const sf::Event::MouseButtonEvent& event, const std::string& symbol) {
    std::cout << symbol << " ";
    switch (event.button) {
        case sf::Mouse::Button::Left:
            std::cout << "Left     ";
            break;
        case sf::Mouse::Button::Right:
            std::cout << "Right    ";
            break;
        case sf::Mouse::Button::Middle:
            std::cout << "Middle   ";
            break;
        case sf::Mouse::Button::XButton1:
            std::cout << "XButton1 ";
            break;
        case sf::Mouse::Button::XButton2:
            std::cout << "XButton2 ";
            break;
        default:
            break;
    }
    std::cout << "(x: " << event.x << ", ";
    std::cout << "y: " << event.y << ")" << std::endl;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "scrollwheel");
    while (true) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::MouseButtonPressed:
                    print_mouse_button_event(event.mouseButton, "⤓");
                    break;
                case sf::Event::MouseButtonReleased:
                    print_mouse_button_event(event.mouseButton, "↥");
                    break;
                default:
                    break;
            }
        }
        
        window.clear();
        window.display();
    }
    return 0;
}

