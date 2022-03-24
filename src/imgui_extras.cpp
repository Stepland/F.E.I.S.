#include "imgui_extras.hpp"

#include <imgui_stdlib.h>

bool feis::ColorEdit4(const char* label, sf::Color& col, ImGuiColorEditFlags flags) {
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

bool feis::InputDecimal(const char *label, Decimal* value) {
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

/*
Imgui::InputText that gets colored red or green depending on isValid.
hoverTextHelp gets displayed when hovering over invalid input.
Displays InputText without any style change if the input is empty;
*/
bool feis::InputTextColored(
    const char* label,
    std::string* str,
    bool isValid,
    const std::string& hoverHelpText
) {
    bool return_value;
    if (str->empty()) {
        return ImGui::InputText(label, str);
    } else {
        if (not isValid) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::red.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::hovered_red.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::active_red.Value);
        } else {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::green.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::hovered_green.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::active_green.Value);
        }
        return_value = ImGui::InputText(label, str);
        if (ImGui::IsItemHovered() and (not isValid)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(hoverHelpText.c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopStyleColor(3);
        return return_value;
    }
}
