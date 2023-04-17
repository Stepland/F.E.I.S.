#pragma once

#include <SFML/System/Vector2.hpp>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <imgui-SFML_export.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "special_numeric_types.hpp"

namespace feis {
    bool ColorEdit4(
        const char* label,
        sf::Color& col,
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaPreviewHalf
    );
    bool InputDecimal(
        const char *label,
        Decimal* value,
        const ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );
    bool InputTextColored(
        const char* label,
        std::string* str,
        bool isValid,
        const std::string& hoverHelpText,
        const ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );
    void HelpMarker(const char* desc);
    bool StopButton(const char* str_id);
    
    template<class T>
    bool IconButton(const char* str_id, const T& draw_icon) {
        float sz = ImGui::GetFrameHeight();
        sf::Vector2f size{sz, sz};
        ImGuiButtonFlags flags = ImGuiButtonFlags_None;
        auto window = ImGui::GetCurrentWindow();
        if (window->SkipItems) {
            return false;
        }

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(str_id);
        const ImRect bb(window->DC.CursorPos, sf::Vector2f(window->DC.CursorPos) + size);
        const float default_size = ImGui::GetFrameHeight();
        ImGui::ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
            flags |= ImGuiButtonFlags_Repeat;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

        // Render
        const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
        ImGui::RenderNavHighlight(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
        draw_icon(bb, text_col);
        return pressed;
    }

    void CenteredText(const std::string& text);

    bool SquareButton(const char* text);
    void ColorSquare(const sf::Color& color);
}

namespace colors {
    const ImColor green = {0.163f, 0.480f, 0.160f, 0.540f};
    const ImColor hovered_green = {0.261f, 0.980f, 0.261f, 0.400f};
    const ImColor active_green = {0.261f, 0.980f, 0.261f, 0.671f};
    const ImColor red = {0.480f, 0.160f, 0.160f, 0.540f};
    const ImColor hovered_red = {0.980f, 0.261f, 0.261f, 0.400f};
    const ImColor active_red = {0.980f, 0.261f, 0.261f, 0.671f};
};