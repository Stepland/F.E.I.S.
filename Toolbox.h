//
// Created by Syméon on 13/01/2019.
//

#ifndef FEIS_TOOLBOX_H
#define FEIS_TOOLBOX_H

#define IM_MAX(_A,_B)       (((_A) >= (_B)) ? (_A) : (_B))

#include <SFML/Window.hpp>
#include <filesystem>

namespace Toolbox {
    bool isShortcutPressed(std::initializer_list<sf::Keyboard::Key> anyOf, std::initializer_list<sf::Keyboard::Key> allOf);
    void pushNewRecentFile(std::filesystem::path path);
    std::vector<std::string> getRecentFiles();

    struct CustomConstraints {
        static void ContentSquare(ImGuiSizeCallbackData* data) {
            float TitlebarHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            float y = data->DesiredSize.y - TitlebarHeight;
            float x = data->DesiredSize.x;
            data->DesiredSize = ImVec2(IM_MAX(x,y), IM_MAX(x,y) + TitlebarHeight);
        }
    };
}

#endif //FEIS_TOOLBOX_H
