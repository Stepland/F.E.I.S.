//
// Created by Syméon on 02/03/2019.
//

#include <imgui.h>
#include "Notification.h"

TextNotification::TextNotification(const std::string &message) : message(message) {}

void TextNotification::display() const {
    ImGui::TextUnformatted(message.c_str());
}

void UndoNotification::display() const {
    ImGui::TextColored(ImVec4(0.928f, 0.611f, 0.000f, 1.000f),"Undo : ");
    ImGui::SameLine();
    ImGui::TextUnformatted(message.c_str());
}

void RedoNotification::display() const {
    ImGui::TextColored(ImVec4(0.000f, 0.595f, 0.734f, 1.000f),"Redo : ");
    ImGui::SameLine();
    ImGui::TextUnformatted(message.c_str());
}
