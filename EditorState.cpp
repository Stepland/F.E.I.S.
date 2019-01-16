//
// Created by Symeon on 23/12/2018.
//

#include <filesystem>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include "EditorState.h"
#include "tinyfiledialogs.h"
#include "Toolbox.h"

EditorState::EditorState(Fumen &fumen) : fumen(fumen), markerEndingState(MISS) {
    reloadFromFumen();
}

void EditorState::reloadFromFumen() {
    if (not this->fumen.Charts.empty()) {
        this->selectedChart = this->fumen.Charts.begin()->second;
    } else {
        this->selectedChart.reset();
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

    ImGui::SetNextWindowSize(ImVec2(400,400),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0,0),ImVec2(FLT_MAX,FLT_MAX),Toolbox::CustomConstraints::ContentSquare);
    if (ImGui::Begin("Playfield",&showPlayfield,ImGuiWindowFlags_NoScrollbar)) {
        float squareSize = ImGui::GetWindowSize().x / 4.f;
        float TitlebarHeight = ImGui::GetWindowSize().y - ImGui::GetWindowSize().x;
        if (selectedChart) {
            int ImGuiIndex = 0;
            for (auto note : getVisibleNotes()) {
                std::optional<sf::Texture> t;
                if ((t = playfield.marker.getSprite(playfield.markerEndingState,getSecondsAt(note.getTiming())))) {
                    int x = note.getPos()%4;
                    int y = note.getPos()/4;
                    ImGui::SetCursorPos({x*squareSize,TitlebarHeight + y*squareSize});
                    ImGui::PushID(ImGuiIndex);
                    ImGui::Image(*t,{squareSize,squareSize});
                    ImGui::PopID();
                }
                ++ImGuiIndex;
            }
        }
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                ImGui::PushID(x+4*y);
                ImGui::SetCursorPos({x*squareSize,TitlebarHeight + y*squareSize});
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0,0,1.f,0.1f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0,0,1.f,0.5f));
                ImGui::ImageButton(playfield.button,{squareSize,squareSize},0);
                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
        }
    }
    ImGui::End();

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
        if (Toolbox::InputTextColored(music.has_value(),"Invalid Music Path","Music",&fumen.musicPath)) {
            reloadMusic();
        }
        if (Toolbox::InputTextColored(jacket.has_value(),"Invalid Jacket Path","Jacket",&(fumen.jacketPath))) {
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
        ImGui::Checkbox("Beat Tick",&playBeatTick);
        ImGui::Checkbox("Note Tick",&playNoteTick);
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
            |ImGuiWindowFlags_NoMove
            |ImGuiWindowFlags_AlwaysAutoResize
            );
    {
        if (selectedChart) {
            ImGui::Text("%s %d",selectedChart->dif_name.c_str(),selectedChart->level);
        } else {
            ImGui::TextDisabled("No chart selected");
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Beats :");
        ImGui::SameLine();
        ImGui::Text("%02.2f",this->getBeats());
        ImGui::SameLine();
        if (music) {
            ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Music File Offset :");
            ImGui::SameLine();
            ImGui::TextUnformatted(Toolbox::to_string(music->getPlayingOffset()).c_str());
            /*
            sf::Time time = music->getPlayingOffset();
            int minutes = static_cast<int>(std::abs(time.asSeconds()))/60;
            int seconds = static_cast<int>(std::abs(time.asSeconds()))%60;
            int miliseconds = static_cast<int>(std::abs(time.asMilliseconds()))%1000;
            if (time.asSeconds() < 0) {
                ImGui::Text("-%02d:%02d.%03d",minutes,seconds,miliseconds);
            } else {
                ImGui::Text("+%02d:%02d.%03d",minutes,seconds,miliseconds);
            }
             */
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Timeline Position :");
        ImGui::SameLine();
        ImGui::TextUnformatted(Toolbox::to_string(playbackPosition).c_str());
        /*
        sf::Time time = playbackPosition;
        int minutes = static_cast<int>(time.asSeconds())/60;
        int seconds = static_cast<int>(time.asSeconds())%60;
        int miliseconds = static_cast<int>(time.asMilliseconds())%1000;
        ImGui::Text("%02d:%02d.%03d",minutes,seconds,miliseconds);
         */
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

void EditorState::displayChartList() {

    if (ImGui::Begin("Chart List",&showChartList,ImGuiWindowFlags_AlwaysAutoResize)) {
        if (this->fumen.Charts.empty()) {
            ImGui::Dummy({100,0}); ImGui::SameLine();
            ImGui::Text("- no charts -"); ImGui::SameLine();
            ImGui::Dummy({100,0});
        } else {
            ImGui::Dummy(ImVec2(300,0));
            ImGui::Columns(3, "mycolumns");
            ImGui::TextDisabled("Difficulty"); ImGui::NextColumn();
            ImGui::TextDisabled("Level"); ImGui::NextColumn();
            ImGui::TextDisabled("Note Count"); ImGui::NextColumn();
            ImGui::Separator();
            for (auto tuple : fumen.Charts) {
                if (ImGui::Selectable(tuple.first.c_str(), selectedChart ? *selectedChart==tuple.second : false , ImGuiSelectableFlags_SpanAllColumns)) {
                    selectedChart = tuple.second;
                }
                ImGui::NextColumn();
                ImGui::Text("%d",tuple.second.level); ImGui::NextColumn();
                ImGui::Text("%d", static_cast<int>(tuple.second.Notes.size())); ImGui::NextColumn();
                ImGui::PushID(&tuple);
                ImGui::PopID();
            }
        }
    }
    ImGui::End();
}

std::vector<Note> EditorState::getVisibleNotes() {
    if (selectedChart) {

        std::vector<Note> visibleNotes;

        float minPos;
        if (this->markerEndingState == MISS) {
            minPos = playbackPosition.asSeconds() - 8.f/30.f;
        } else {
            minPos = playbackPosition.asSeconds() - 16.f/30.f;
        }

        float maxPos = playbackPosition.asSeconds() + 16.f/30.f;

        int min_exclusive = static_cast<int>(getTicksAt(minPos));
        int max_exclusive = static_cast<int>(getTicksAt(maxPos));

        for (auto note : selectedChart->Notes) {
            if (note.getTiming() > min_exclusive and note.getTiming() < max_exclusive) {
                visibleNotes.push_back(note);
            }
        }

        return visibleNotes;

    } else {
        return {};
    }
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

/*
 * Returns the newly created chart if there is
 */
std::optional<Chart> ESHelper::NewChartDialog::display(EditorState &editorState) {

    std::optional<Chart> newChart;
    if (ImGui::Begin(
            "New Chart",
            &editorState.showNewChartDialog,
            ImGuiWindowFlags_NoResize
            |ImGuiWindowFlags_AlwaysAutoResize))
    {

        if (showCustomDifName) {
            comboPreview = "Custom";
        } else {
            if (difficulty.empty()) {
                comboPreview = "Choose One";
            } else {
                comboPreview = difficulty;
            }
        }
        if(ImGui::BeginCombo("Difficulty",comboPreview.c_str())) {
            for (auto dif_name : {"BSC","ADV","EXT"}) {
                if (editorState.fumen.Charts.find(dif_name) == editorState.fumen.Charts.end()) {
                    if(ImGui::Selectable(dif_name,dif_name == difficulty)) {
                        showCustomDifName = false;
                        difficulty = dif_name;
                    }
                } else {
                    ImGui::TextDisabled(dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom",&showCustomDifName)) {
                difficulty = "";
            }
            ImGui::EndCombo();
        }
        if (showCustomDifName) {
            Toolbox::InputTextColored(
                    editorState.fumen.Charts.find(difficulty) == editorState.fumen.Charts.end(),
                    "Chart name has to be unique",
                    "Difficulty Name",
                    &difficulty
                    );
        }
        ImGui::InputInt("Level",&level);
        ImGui::Separator();
        if (ImGui::TreeNode("Advanced##New Chart")) {
            ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
            if (ImGui::InputInt("Resolution",&resolution)) {
                if (resolution < 1) {
                    resolution = 1;
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Number of ticks in a beat");
                ImGui::BulletText("Has nothing to do with time signature");
                ImGui::BulletText("Leave the default unless you know what you're doing");
                ImGui::EndTooltip();
            }
            ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
            ImGui::TreePop();
        }
        ImGui::Separator();
        if (difficulty.empty() or (editorState.fumen.Charts.find(difficulty) != editorState.fumen.Charts.end())) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Create Chart##New Chart");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Create Chart##New Chart")) {
                try {
                    newChart.emplace(difficulty,level,resolution);
                } catch (const std::exception& e) {
                    tinyfd_messageBox("Error",e.what(),"ok","error",1);
                }
            }
        }
    }
    ImGui::End();
    return newChart;
}

void ESHelper::ChartPropertiesDialog::display(EditorState &editorState) {

    assert(editorState.selectedChart.has_value());

    if (this->shouldRefreshValues) {

        shouldRefreshValues = false;

        difNamesInUse.clear();
        this->level = editorState.selectedChart->level;
        this->difficulty = editorState.selectedChart->dif_name;
        std::set<std::string> difNames{"BSC","ADV","EXT"};
        showCustomDifName = (difNames.find(difficulty) == difNames.end());

        for (auto const& tuple : editorState.fumen.Charts) {
            if (tuple.second != editorState.selectedChart) {
                difNamesInUse.insert(tuple.first);
            }
        }
    }

    if (ImGui::Begin(
            "Chart Properties",
            &editorState.showChartProperties,
            ImGuiWindowFlags_NoResize
            |ImGuiWindowFlags_AlwaysAutoResize))
    {

        if (showCustomDifName) {
            comboPreview = "Custom";
        } else {
            if (difficulty.empty()) {
                comboPreview = "Choose One";
            } else {
                comboPreview = difficulty;
            }
        }
        if(ImGui::BeginCombo("Difficulty",comboPreview.c_str())) {
            for (auto dif_name : {"BSC","ADV","EXT"}) {
                if (difNamesInUse.find(dif_name) == difNamesInUse.end()) {
                    if(ImGui::Selectable(dif_name,dif_name == difficulty)) {
                        showCustomDifName = false;
                        difficulty = dif_name;
                    }
                } else {
                    ImGui::TextDisabled(dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom",&showCustomDifName)) {
                difficulty = "";
            }
            ImGui::EndCombo();
        }
        if (showCustomDifName) {
            Toolbox::InputTextColored(
                    difNamesInUse.find(difficulty) == difNamesInUse.end(),
                    "Chart name has to be unique",
                    "Difficulty Name",
                    &difficulty
            );
        }
        ImGui::InputInt("Level",&level);
        ImGui::Separator();
        if (difficulty.empty() or (difNamesInUse.find(difficulty) != difNamesInUse.end())) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Apply##New Chart");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Apply##New Chart")) {
                try {
                    editorState.fumen.Charts.erase(editorState.selectedChart->dif_name);
                    editorState.selectedChart->dif_name = this->difficulty;
                    editorState.selectedChart->level = this->level;
                    if (not (editorState.fumen.Charts.emplace(this->difficulty,editorState.selectedChart.value())).second) {
                        throw std::runtime_error("Could not insert modified chart in fumen");
                    } else {
                        shouldRefreshValues = true;
                    }
                } catch (const std::exception& e) {
                    tinyfd_messageBox("Error",e.what(),"ok","error",1);
                }
            }
        }
    }
    ImGui::End();

}
