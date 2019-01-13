//
// Created by Syméon on 13/01/2019.
//

#ifndef FEIS_TOOLBOX_H
#define FEIS_TOOLBOX_H

#include <SFML/Window.hpp>
#include <filesystem>

namespace Toolbox {
    bool isShortcutPressed(std::initializer_list<sf::Keyboard::Key> anyOf, std::initializer_list<sf::Keyboard::Key> allOf);
    void pushNewRecentFile(std::filesystem::path path);
    std::vector<std::string> getRecentFiles();
}

#endif //FEIS_TOOLBOX_H
