#include "imgui_extras.hpp"

#include <SFML/System/Vector2.hpp>
#include <imgui.h>

#include <imgui_stdlib.h>
#include "imgui_internal.h"

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

bool feis::InputDecimal(
    const char *label,
    Decimal* value,
    const ImGuiInputTextFlags flags
) {
    auto s = value->format("f");
    if (ImGui::InputText(label, &s, flags | ImGuiInputTextFlags_CharsDecimal)) {
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
bool feis::InputTextWithErrorTooltip(
    const char* label,
    std::string* str,
    bool isValid,
    const std::string& hoverHelpText,
    const ImGuiInputTextFlags flags
) {
    bool return_value;
    if (str->empty()) {
        return ImGui::InputText(label, str, flags);
    } else {
        
        if (not isValid) {
            return_value = InputTextColored(label, str, input_colors::red, flags);
        } else {
            return_value = InputTextColored(label, str, input_colors::green, flags);
        }
        if (ImGui::IsItemHovered() and (not isValid)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(hoverHelpText.c_str());
            ImGui::EndTooltip();
        }
        return return_value;
    }
}

bool feis::InputTextColored(
    const char* label,
    std::string* str,
    const input_colors::InputBoxColor& colors_,
    const ImGuiInputTextFlags flags
) {
    bool return_value;
    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors_.normal);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors_.hovered);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors_.active);
    return_value = ImGui::InputText(label, str, flags);
    ImGui::PopStyleColor(3);
    return return_value;
}

void feis::HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool feis::StopButton(const char* str_id) {
    return IconButton(str_id, [](const ImRect& bb, const ImU32& color){
        const auto size = sf::Vector2f{bb.GetSize()};
        ImGui::GetWindowDrawList()->AddRectFilled(
            sf::Vector2f{bb.Min} + size / 4.f,
            sf::Vector2f{bb.Max} - size / 4.f + sf::Vector2f{1.f, 1.f},
            color
        );
    });
}

void feis::CenteredText(const std::string& text) {
    const auto c_str = text.c_str();
    const auto window_width = ImGui::GetWindowSize().x;
    const auto text_width = ImGui::CalcTextSize(c_str).x;
    ImGui::SetCursorPosX((window_width - text_width) / 2);
    ImGui::TextUnformatted(c_str);
}

bool feis::SquareButton(const char* text) {
    const auto button_size = ImGui::GetFrameHeight();
    return ImGui::ButtonEx(text, ImVec2(button_size, button_size));
}

void feis::ColorSquare(const sf::Color& color) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float sz = ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImColor(color));
    ImGui::Dummy(ImVec2(sz, sz));
}

void feis::ColorDot(const sf::Color& color) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float sz = ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddCircleFilled({p.x + sz / 2.0f, p.y + sz/2.0f}, sz / 2.0f, ImColor(color));
    ImGui::Dummy(ImVec2(sz, sz));
}
