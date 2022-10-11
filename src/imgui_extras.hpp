#pragma once

#include <string>

#include <SFML/Graphics/Color.hpp>
#include <imgui-SFML_export.h>
#include <imgui.h>

#include "special_numeric_types.hpp"

namespace feis {
    bool ColorEdit4(const char* label, sf::Color& col, ImGuiColorEditFlags flags = 0);
    bool InputDecimal(const char *label, Decimal* value);
    bool InputTextColored(const char* label, std::string* str, bool isValid, const std::string& hoverHelpText);
    void HelpMarker(const char* desc);
}

namespace colors {
    const ImColor green = {0.163f, 0.480f, 0.160f, 0.540f};
    const ImColor hovered_green = {0.261f, 0.980f, 0.261f, 0.400f};
    const ImColor active_green = {0.261f, 0.980f, 0.261f, 0.671f};
    const ImColor red = {0.480f, 0.160f, 0.160f, 0.540f};
    const ImColor hovered_red = {0.980f, 0.261f, 0.261f, 0.400f};
    const ImColor active_red = {0.980f, 0.261f, 0.261f, 0.671f};
};