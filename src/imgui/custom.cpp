#include "custom.hpp"

bool ImGui::ColorEdit4(const char* label, sf::Color& col, ImGuiColorEditFlags flags) {

    float array_col[4] = {
        static_cast<float>(col.r)/255.f,
        static_cast<float>(col.g)/255.f,
        static_cast<float>(col.b)/255.f,
        static_cast<float>(col.a)/255.f
    };

    if (ImGui::ColorEdit4(label, array_col, flags)) {
        col.r = static_cast<sf::Uint8>(array_col[0]*255.f);
        col.g = static_cast<sf::Uint8>(array_col[1]*255.f);
        col.b = static_cast<sf::Uint8>(array_col[2]*255.f);
        col.a = static_cast<sf::Uint8>(array_col[3]*255.f);
        return true;
    }
    return false;
}
