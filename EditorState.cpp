//
// Created by Symeon on 23/12/2018.
//

#include <filesystem>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include "EditorState.h"

EditorState::EditorState(Fumen &fumen) : fumen(fumen) {
    if (not this->fumen.Charts.empty()) {
        this->selectedChart = this->fumen.Charts.begin()->first;
    }
    reloadMusic();
    reloadJacket();
}

/*
 * Reloads music from what's indicated in the "music path" field of the fumen
 * Resets the music state in case anything fails
 */
void EditorState::reloadMusic() {
    music.emplace();
    try {
        if (!music->openFromFile(
                (fumen.path.parent_path() / std::filesystem::path(fumen.musicPath)).string())
                ) {
            music.reset();
        }
    } catch (const std::exception& e) {
        music.reset();
    }
}

/*
 * Reloads the jacket from what's indicated in the "jacket path" field of the fumen
 * Resets the jacket state if anything fails
 */
void EditorState::reloadJacket() {
    jacket.emplace();
    try {
        if (!jacket->loadFromFile((fumen.path.parent_path() / std::filesystem::path(fumen.jacketPath)).string())) {
            jacket.reset();
        }
    } catch (const std::exception& e) {
        jacket.reset();
    }

}

/*
 * Display all metadata in an editable form
 */
void EditorState::displayProperties() {
    ImGui::SetNextWindowSize(ImVec2(500,240));
    ImGui::Begin("Properties",&showProperties,ImGuiWindowFlags_NoResize);
    {
        ImGui::Columns(2, nullptr, false);

        if (jacket) {
            ImGui::Image(*jacket,sf::Vector2f(200,200));
        } else {
            ImGui::BeginChild("Jacket",ImVec2(200,200),true);
            ImGui::EndChild();
        }


        ImGui::NextColumn();

        ImGui::InputText("Title",&fumen.songTitle);
        ImGui::InputText("Artist",&fumen.artist);
        if (ImGui::InputText("Music",&fumen.musicPath)) {
            reloadMusic();
        };
        if (ImGui::InputText("Jacket",&(fumen.jacketPath))) {
            reloadJacket();
        }
    }
    ImGui::End();
}

/*
 * Display any information that would be useful for the user to troubleshoot the status of the editor
 * will appear in the "Editor Status" window
 */
void EditorState::displayStatus() {
    ImGui::Begin("Status",&showStatus);
    {
        if (not music) {
            if (not fumen.musicPath.empty()) {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"Invalid music path : %s",fumen.musicPath.c_str());
            } else {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"No music file loaded");
            }
        }

        if (not jacket) {
            if (not fumen.jacketPath.empty()) {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"Invalid jacket path : %s",fumen.jacketPath.c_str());
            } else {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"No jacket loaded");
            }
        }
    }
    ImGui::End();
}

void EditorState::displayPlaybackStatus() {

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 25), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
    ImGui::Begin(
            "Playback Status",
            &showPlaybackStatus,
            ImGuiWindowFlags_NoNav
            |ImGuiWindowFlags_NoDecoration
            |ImGuiWindowFlags_NoInputs
            |ImGuiWindowFlags_NoTitleBar
            |ImGuiWindowFlags_NoMove
            );
    {
        if (music) {
            ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Time : ");
            ImGui::SameLine();
            sf::Time time = music->getPlayingOffset();
            int minutes = static_cast<int>(time.asSeconds())/60;
            int seconds = static_cast<int>(time.asSeconds())%60;
            int miliseconds = static_cast<int>(time.asMilliseconds())%1000;
            ImGui::Text("%02d:%02d.%03d",minutes,seconds,miliseconds);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
