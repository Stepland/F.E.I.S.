#ifndef FEIS_IMGUI_CUSTOM_H
#define FEIS_IMGUI_CUSTOM_H

#include <SFML/Graphics/Color.hpp>
#include <imgui-SFML_export.h>
#include <imgui.h>

// My own custom SFML overloads for ImGui
namespace ImGui {
    IMGUI_SFML_API bool ColorEdit4(const char* label, sf::Color& col, ImGuiColorEditFlags flags = 0);
}

#endif  // FEIS_IMGUI_CUSTOM_H