//
// Created by Symeon on 23/12/2018.
//

#include <filesystem>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include "EditorState.h"
#include "tinyfiledialogs.h"
#include "Toolbox.h"

EditorState::EditorState(Fumen &fumen) : fumen(fumen) {
    reloadFromFumen();
}

void EditorState::reloadFromFumen() {
    if (not this->fumen.Charts.empty()) {
        this->selectedChart = this->fumen.Charts.begin()->second;
    }
    reloadMusic();
    reloadJacket();
}

/*
 * Reloads music from what's indicated in the "music path" field of the fumen
 * Resets the music state in case anything fails
 * Updates playbackPosition and the chartRuntime
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
    reloadPlaybackPositionAndChartRuntime();
}

/*
 * NEVER CALL THAT YOURSELF,
 * Let reloadMusic do it,
 * you can end up with some strange stuff if you call it before reloadMusic
 */
void EditorState::reloadPlaybackPositionAndChartRuntime() {
    playbackPosition = sf::seconds(-(fumen.offset));
    if (music) {
        if (selectedChart) {
            chartRuntime = sf::seconds(std::max(music->getDuration().asSeconds(),fumen.getChartRuntime(*selectedChart)-fumen.offset)+2.f);
        } else {
            chartRuntime = sf::seconds(std::max(-fumen.offset,music->getDuration().asSeconds()));
        }
    } else {
        if (selectedChart) {
            chartRuntime = sf::seconds(std::max(fumen.getChartRuntime(*selectedChart)-fumen.offset,2.f));
        } else {
            chartRuntime = sf::seconds(std::max(-fumen.offset,2.f));
        }
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

void EditorState::displayPlayfield() {
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
            |ImGuiWindowFlags_AlwaysAutoResize
            );
    {
        if (music) {
            ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Music File Offset : ");
            ImGui::SameLine();
            sf::Time time = music->getPlayingOffset();
            int minutes = static_cast<int>(time.asSeconds())/60;
            int seconds = static_cast<int>(time.asSeconds())%60;
            int miliseconds = static_cast<int>(time.asMilliseconds())%1000;
            ImGui::Text("%02d:%02d.%03d",minutes,seconds,miliseconds);
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Timeline Position : ");
        ImGui::SameLine();
        sf::Time time = playbackPosition;
        int minutes = static_cast<int>(time.asSeconds())/60;
        int seconds = static_cast<int>(time.asSeconds())%60;
        int miliseconds = static_cast<int>(time.asMilliseconds())%1000;
        ImGui::Text("%02d:%02d.%03d",minutes,seconds,miliseconds);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorState::displayTimeline() {

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 25, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize({20,io.DisplaySize.y * 0.9f},ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0);
    ImGui::Begin(
            "Timeline",
            &showTimeline,
            ImGuiWindowFlags_NoNav
            |ImGuiWindowFlags_NoDecoration
            |ImGuiWindowFlags_NoTitleBar
            |ImGuiWindowFlags_NoMove
            );
    {
        if (music) {
            AffineTransform<float> scroll(-fumen.offset,chartRuntime.asSeconds(),1.f,0.f);
            float slider_pos = scroll.transform(playbackPosition.asSeconds());
            ImGui::SetCursorPos({0,0});
            if(ImGui::VSliderFloat("",ImGui::GetContentRegionMax(),&slider_pos,0.f,1.f,"")) {
                playbackPosition = sf::seconds(scroll.backwards_transform(slider_pos));
                if (playbackPosition.asSeconds() >= 0 and playbackPosition < music->getDuration()) {
                    music->setPlayingOffset(playbackPosition);
                }
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
}

void ESHelper::save(EditorState& ed) {
    try {
        ed.fumen.autoSaveAsMemon();
    } catch (const std::exception& e) {
        tinyfd_messageBox("Error",e.what(),"ok","error",1);
    }
}

void ESHelper::open(std::optional<EditorState> &ed) {
    const char* _filepath = tinyfd_openFileDialog("Open File",nullptr,0,nullptr,nullptr,false);
    if (_filepath != nullptr) {
        ESHelper::openFromFile(ed,_filepath);
    }
}

void ESHelper::openFromFile(std::optional<EditorState> &ed, std::filesystem::path path) {
    try {
        Fumen f(path);
        f.autoLoadFromMemon();
        ed.emplace(f);
        Toolbox::pushNewRecentFile(std::filesystem::canonical(ed->fumen.path));
    } catch (const std::exception &e) {
        tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
    }
}
