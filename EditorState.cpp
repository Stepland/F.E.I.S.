//
// Created by Symeon on 23/12/2018.
//

#include <cmath>
#include <filesystem>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include "EditorState.h"
#include "tinyfiledialogs.h"
#include "Toolbox.h"

EditorState::EditorState(Fumen &fumen) : fumen(fumen) {
    reloadFromFumen();
}

void EditorState::reloadFromFumen() {
    if (not this->fumen.Charts.empty()) {
        this->chart.emplace(this->fumen.Charts.begin()->second);
    } else {
        this->chart.reset();
    }
    reloadMusic();
    reloadAlbumCover();
}

/*
 * Reloads music from what's indicated in the "music path" field of the fumen
 * Resets the music state in case anything fails
 * Updates playbackPosition and previewEnd as well
 */
void EditorState::reloadMusic() {

    music.emplace();

    std::filesystem::path music_path = std::filesystem::path(fumen.path).parent_path() / fumen.musicPath;

    if (
        fumen.musicPath.empty() or
        not std::filesystem::exists(music_path) or
        not music->openFromFile(music_path.string())
    ) {
        music.reset();
    }

    playbackPosition = sf::seconds(-(fumen.offset));
    previousPos = playbackPosition;
    if (music) {
        if (chart) {
            previewEnd = sf::seconds(std::max(music->getDuration().asSeconds(),fumen.getChartRuntime(chart->ref)-fumen.offset)+2.f);
        } else {
            previewEnd = sf::seconds(std::max(-fumen.offset,music->getDuration().asSeconds()));
        }
    } else {
        if (chart) {
            previewEnd = sf::seconds(std::max(fumen.getChartRuntime(chart->ref)-fumen.offset,2.f));
        } else {
            previewEnd = sf::seconds(std::max(-fumen.offset,2.f));
        }
    }
}

/*
 * Reloads the album cover from what's indicated in the "album cover path" field of the fumen
 * Resets the album cover state if anything fails
 */
void EditorState::reloadAlbumCover() {

    albumCover.emplace();

    std::filesystem::path album_cover_path = std::filesystem::path(fumen.path).parent_path() / fumen.albumCoverPath;

    if (
        fumen.albumCoverPath.empty() or
        not std::filesystem::exists(album_cover_path) or
        not albumCover->loadFromFile(album_cover_path.string())
    ) {
        albumCover.reset();
    }

}

void EditorState::setPlaybackAndMusicPosition(sf::Time newPosition) {

    if (newPosition.asSeconds() < -fumen.offset) {
        newPosition = sf::seconds(-fumen.offset);
    } else if (newPosition > previewEnd) {
        newPosition = previewEnd;
    }
    previousPos = sf::seconds(newPosition.asSeconds() - 1.f/60.f);
    playbackPosition = newPosition;
    if (music) {
        if (playbackPosition.asSeconds() >= 0 and playbackPosition < music->getDuration()) {
            music->setPlayingOffset(playbackPosition);
        } else {
            music->stop();
        }
    }
}

void EditorState::displayPlayfield(Marker& marker, MarkerEndingState markerEndingState) {

    ImGui::SetNextWindowSize(ImVec2(400,400),ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0,0),ImVec2(FLT_MAX,FLT_MAX),Toolbox::CustomConstraints::ContentSquare);

    if (ImGui::Begin("Playfield",&showPlayfield,ImGuiWindowFlags_NoScrollbar)) {

        float squareSize = ImGui::GetWindowSize().x / 4.f;
        float TitlebarHeight = ImGui::GetWindowSize().y - ImGui::GetWindowSize().x;
        int ImGuiIndex = 0;

        if (chart) {

            for (auto const& note : visibleNotes) {

                float note_offset = (playbackPosition.asSeconds() - getSecondsAt(note.getTiming()));
                auto frame = static_cast<long long int>(std::floor(note_offset * 30.f));
                int x = note.getPos() % 4;
                int y = note.getPos() / 4;


                if (note.getLength() == 0) {

                    // Display normal notes

                    auto t = marker.getSprite(markerEndingState, note_offset);

                    if (t) {
                        ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                        ImGui::PushID(ImGuiIndex);
                        ImGui::Image(*t, {squareSize, squareSize});
                        ImGui::PopID();
                        ++ImGuiIndex;
                    }

                } else {

                    // Display long notes

                    float tail_end_in_seconds = getSecondsAt(note.getTiming() + note.getLength());
                    float tail_end_offset = playbackPosition.asSeconds() - tail_end_in_seconds;

                    if (playbackPosition.asSeconds() < tail_end_in_seconds) {

                        // Before or During the long note

                        int triangle = note.getTail_pos_as_note_pos();

                        auto triangle_x = static_cast<float>(triangle % 4);
                        auto triangle_y = static_cast<float>(triangle / 4);

                        AffineTransform<float> x_trans(0.0f, ticksToSeconds(note.getLength()), triangle_x,
                                                       static_cast<float>(x));
                        AffineTransform<float> y_trans(0.0f, ticksToSeconds(note.getLength()), triangle_y,
                                                       static_cast<float>(y));
                        triangle_x = x_trans.clampedTransform(note_offset);
                        triangle_y = y_trans.clampedTransform(note_offset);

                        auto tail_tex = playfield.longNoteMarker.getTailTexture(note_offset, note.getTail_pos());
                        if (tail_tex) {

                            ImVec2 cursorPos;
                            sf::Vector2f texSize;

                            if (frame < 8) {

                                // Before the note : tail goes from triangle tip to note edge

                                switch (note.getTail_pos() % 4) {

                                    // going down
                                    case 0:
                                        cursorPos.x = x * squareSize;
                                        cursorPos.y = (triangle_y + 1) * squareSize;
                                        texSize.x = squareSize;
                                        texSize.y = (y - triangle_y - 1) * squareSize;
                                        break;

                                        // going left (to the left, to the left ...)
                                    case 1:
                                        cursorPos.x = (x + 1) * squareSize;
                                        cursorPos.y = y * squareSize;
                                        texSize.x = (triangle_x - x - 1) * squareSize;
                                        texSize.y = squareSize;
                                        break;

                                        // going up
                                    case 2:
                                        cursorPos.x = x * squareSize;
                                        cursorPos.y = (y + 1) * squareSize;
                                        texSize.x = squareSize;
                                        texSize.y = (triangle_y - y - 1) * squareSize;
                                        break;

                                        // going right
                                    case 3:
                                        cursorPos.x = (triangle_x + 1) * squareSize;
                                        cursorPos.y = y * squareSize;
                                        texSize.x = (x - triangle_x - 1) * squareSize;
                                        texSize.y = squareSize;
                                        break;

                                    default:
                                        throw std::runtime_error("wtf ?");
                                }

                            } else {

                                // During the note : tail goes from triangle base to note edge

                                switch (note.getTail_pos() % 4) {

                                    // going down
                                    case 0:
                                        cursorPos.x = x * squareSize;
                                        cursorPos.y = (triangle_y + 0.9f) * squareSize;
                                        texSize.x = squareSize;
                                        texSize.y = (y - triangle_y - 0.9f) * squareSize;
                                        break;

                                        // going left (to the left, to the left ...)
                                    case 1:
                                        cursorPos.x = (x + 1) * squareSize;
                                        cursorPos.y = y * squareSize;
                                        texSize.x = (triangle_x - x - 0.9f) * squareSize;
                                        texSize.y = squareSize;
                                        break;

                                        // going up
                                    case 2:
                                        cursorPos.x = x * squareSize;
                                        cursorPos.y = (y + 1) * squareSize;
                                        texSize.x = squareSize;
                                        texSize.y = (triangle_y - y - 0.9f) * squareSize;
                                        break;

                                        // going right
                                    case 3:
                                        cursorPos.x = (triangle_x + 0.9f) * squareSize;
                                        cursorPos.y = y * squareSize;
                                        texSize.x = (x - triangle_x - 0.9f) * squareSize;
                                        texSize.y = squareSize;
                                        break;

                                    default:
                                        throw std::runtime_error("wtf ?");
                                }

                            }

                            cursorPos.y += TitlebarHeight;

                            ImGui::SetCursorPos(cursorPos);
                            ImGui::PushID(ImGuiIndex);
                            ImGui::Image(*tail_tex, texSize);
                            ImGui::PopID();
                            ++ImGuiIndex;

                            Toolbox::displayIfHasValue(
                                    playfield.longNoteMarker.getSquareBackgroundTexture(note_offset,
                                                                                        note.getTail_pos()),
                                    {x * squareSize, TitlebarHeight + y * squareSize},
                                    {squareSize, squareSize},
                                    ImGuiIndex
                            );

                            Toolbox::displayIfHasValue(
                                    playfield.longNoteMarker.getSquareOutlineTexture(note_offset, note.getTail_pos()),
                                    {x * squareSize, TitlebarHeight + y * squareSize},
                                    {squareSize, squareSize},
                                    ImGuiIndex
                            );

                            Toolbox::displayIfHasValue(
                                    playfield.longNoteMarker.getTriangleTexture(note_offset, note.getTail_pos()),
                                    {triangle_x * squareSize, TitlebarHeight + triangle_y * squareSize},
                                    {squareSize, squareSize},
                                    ImGuiIndex
                            );

                            Toolbox::displayIfHasValue(
                                    playfield.longNoteMarker.getSquareHighlightTexture(note_offset, note.getTail_pos()),
                                    {x * squareSize, TitlebarHeight + y * squareSize},
                                    {squareSize, squareSize},
                                    ImGuiIndex
                            );

                            // Display the beginning marker
                            auto t = marker.getSprite(markerEndingState, note_offset);
                            if (t) {
                                ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                                ImGui::PushID(ImGuiIndex);
                                ImGui::Image(*t, {squareSize, squareSize});
                                ImGui::PopID();
                                ++ImGuiIndex;
                            }
                        }

                    } else {

                        // After long note end : Display the ending marker
                        if (tail_end_offset > 0.0f) {
                            auto t = marker.getSprite(markerEndingState, tail_end_offset);
                            if (t) {
                                ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                                ImGui::PushID(ImGuiIndex);
                                ImGui::Image(*t, {squareSize, squareSize});
                                ImGui::PopID();
                                ++ImGuiIndex;
                            }
                        }
                    }
                }
            }
        }

        // Display button grid
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                ImGui::PushID(x+4*y);
                ImGui::SetCursorPos({x*squareSize,TitlebarHeight + y*squareSize});
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0,0,1.f,0.1f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0,0,1.f,0.5f));
                if (ImGui::ImageButton(playfield.button,{squareSize,squareSize},0)) {
                    toggleNoteAtCurrentTime(x+4*y);
                }
                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
        }

        if (chart) {

            // Check for collisions then display them
            int ticks_threshold = static_cast<int>((1.f/60.f)*fumen.BPM*getResolution());

            std::array<bool, 16> collisions = {};

            for (auto const& note : visibleNotes) {
                if (chart->ref.is_colliding(note,ticks_threshold)) {
                    collisions[note.getPos()] = true;
                }
            }
            for (int i = 0; i < 16; ++i) {
                if (collisions.at(i)) {
                    int x = i%4;
                    int y = i/4;
                    ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                    ImGui::PushID(ImGuiIndex);
                    ImGui::Image(playfield.note_collision, {squareSize, squareSize});
                    ImGui::PopID();
                    ++ImGuiIndex;
                }
            }

            // Display selected notes
            for (auto const& note : visibleNotes) {
                if (chart->selectedNotes.find(note) != chart->selectedNotes.end()) {
                    int x = note.getPos()%4;
                    int y = note.getPos()/4;
                    ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                    ImGui::PushID(ImGuiIndex);
                    ImGui::Image(playfield.note_selected, {squareSize, squareSize});
                    ImGui::PopID();
                    ++ImGuiIndex;
                }
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

        if (albumCover) {
            ImGui::Image(*albumCover,sf::Vector2f(200,200));
        } else {
            ImGui::BeginChild("Album Cover",ImVec2(200,200),true);
            ImGui::EndChild();
        }


        ImGui::NextColumn();
        ImGui::InputText("Title",&fumen.songTitle);
        ImGui::InputText("Artist",&fumen.artist);
        if (Toolbox::InputTextColored(music.has_value(),"Invalid Music Path","Music",&fumen.musicPath)) {
            reloadMusic();
        }
        if (Toolbox::InputTextColored(albumCover.has_value(),"Invalid Album Cover Path","Album Cover",&fumen.albumCoverPath)) {
            reloadAlbumCover();
        }
        if(ImGui::InputFloat("BPM",&fumen.BPM,1.0f,10.0f)) {
            if (fumen.BPM <= 0.0f) {
                fumen.BPM = 0.0f;
            }
        }
        ImGui::InputFloat("offset",&fumen.offset,0.01f,1.f);
    }
    ImGui::End();
}

/*
 * Display any information that would be useful for the user to troubleshoot the status of the editor
 * will appear in the "Editor Status" window
 */
void EditorState::displayStatus() {
    ImGui::Begin("Status",&showStatus,ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (not music) {
            if (not fumen.musicPath.empty()) {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"Invalid music path : %s",fumen.musicPath.c_str());
            } else {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"No music file loaded");
            }
        }

        if (not albumCover) {
            if (not fumen.albumCoverPath.empty()) {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"Invalid albumCover path : %s",fumen.albumCoverPath.c_str());
            } else {
                ImGui::TextColored(ImVec4(1,0.42,0.41,1),"No albumCover loaded");
            }
        }
        if (ImGui::SliderInt("Music Volume",&musicVolume,0,10)) {
            setMusicVolume(musicVolume);
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
            |ImGuiWindowFlags_NoMove
            |ImGuiWindowFlags_AlwaysAutoResize
            );
    {
        if (chart) {
            ImGui::Text("%s %d",chart->ref.dif_name.c_str(),chart->ref.level); ImGui::SameLine();
        } else {
            ImGui::TextDisabled("No chart selected"); ImGui::SameLine();
        }
        ImGui::TextDisabled("Snap : "); ImGui::SameLine();
        ImGui::Text("%s",Toolbox::toOrdinal(snap*4).c_str()); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Beats :"); ImGui::SameLine();
        ImGui::Text("%02.2f",this->getBeats()); ImGui::SameLine();
        if (music) {
            ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Music File Offset :"); ImGui::SameLine();
            ImGui::TextUnformatted(Toolbox::to_string(music->getPlayingOffset()).c_str()); ImGui::SameLine();
        }
        ImGui::TextColored(ImVec4(0.53,0.53,0.53,1),"Timeline Position :"); ImGui::SameLine();
        ImGui::TextUnformatted(Toolbox::to_string(playbackPosition).c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorState::displayTimeline() {

    ImGuiIO& io = ImGui::GetIO();

    float height = io.DisplaySize.y * 0.9f;

    if (chart) {
        if (densityGraph.should_recompute) {
            densityGraph.should_recompute = false;
            densityGraph.computeDensities(static_cast<int>(height), getChartRuntime(), chart->ref, fumen.BPM,
                                          getResolution());
        } else {
            if (densityGraph.last_height) {
                if (static_cast<int>(height) != *(densityGraph.last_height)) {
                    densityGraph.computeDensities(static_cast<int>(height), getChartRuntime(), chart->ref, fumen.BPM,
                                                  getResolution());
                }
            } else {
                densityGraph.computeDensities(static_cast<int>(height), getChartRuntime(), chart->ref, fumen.BPM,
                                              getResolution());
            }
        }
    }

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 35, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize({45,height},ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,1);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0,1.0,1.1,1.0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.240f, 0.520f, 0.880f, 0.500f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.240f, 0.520f, 0.880f, 0.700f));
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
            ImGui::SetCursorPos({0,0});
            ImGui::Image(densityGraph.graph.getTexture(),ImVec2(0,1),ImVec2(1,0));
            AffineTransform<float> scroll(-fumen.offset,previewEnd.asSeconds(),1.f,0.f);
            float slider_pos = scroll.transform(playbackPosition.asSeconds());
            ImGui::SetCursorPos({0,0});
            if(ImGui::VSliderFloat("",ImGui::GetContentRegionMax(),&slider_pos,0.f,1.f,"")) {
                setPlaybackAndMusicPosition(sf::seconds(scroll.backwards_transform(slider_pos)));
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(6);
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
            for (auto& tuple : fumen.Charts) {
                if (ImGui::Selectable(tuple.first.c_str(), chart ? chart->ref==tuple.second : false , ImGuiSelectableFlags_SpanAllColumns)) {
                    chart.emplace(tuple.second);
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

void EditorState::displayLinearView() {

    ImGui::SetNextWindowSize(ImVec2(204,400),ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(204,204),ImVec2(FLT_MAX,FLT_MAX));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(2,2));
    if (ImGui::Begin("Linear View", &showLinearView,ImGuiWindowFlags_NoScrollbar)) {
        if (chart) {
            linearView.update(chart->ref, chart->selectedNotes, chart->timeSelection, playbackPosition, getTicks(), fumen.BPM, getResolution(), ImGui::GetContentRegionMax());
            ImGui::SetCursorPos({0,ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f});
            ImGui::Image(linearView.view.getTexture(),ImVec2(0,1),ImVec2(1,0));
        } else {
            ImGui::TextDisabled("- no chart selected -");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

/*
 * This SCREAAAAMS for optimisation, but in the meantime it works !
 */
void EditorState::updateVisibleNotes() {

    visibleNotes.clear();

    if (chart) {

        float position = playbackPosition.asSeconds();

        for (auto const& note : chart->ref.Notes) {

            float note_timing_in_seconds = getSecondsAt(note.getTiming());

            // we can leave early if the note is happening too far after the position
            if (position > note_timing_in_seconds - 16.f/30.f) {
                if (note.getLength() == 0) {
                    if (position < note_timing_in_seconds + 16.f/30.f) {
                        visibleNotes.insert(note);
                    }
                } else {
                    float tail_end_in_seconds = getSecondsAt(note.getTiming()+note.getLength());
                    if (position < tail_end_in_seconds + 16.f/30.f) {
                        visibleNotes.insert(note);
                    }
                }
            }
        }
    }
}

/*
 * If a note is visible for the given pos, delete it
 * Otherwise create note at nearest tick
 */
void EditorState::toggleNoteAtCurrentTime(int pos) {

    if (chart) {

        std::set<Note> toggledNotes = {};

        bool deleted_something = false;
        for (auto note : visibleNotes) {
            if (note.getPos() == pos) {
                toggledNotes.insert(note);
                chart->ref.Notes.erase(note);
                deleted_something = true;
                break;
            }
        }
        if (not deleted_something) {
            toggledNotes.emplace(pos,static_cast<int>(roundf(getTicks())));
            chart->ref.Notes.emplace(pos,static_cast<int>(roundf(getTicks())));
        }

        chart->history.push(std::make_shared<ToggledNotes>(toggledNotes, not deleted_something));
        densityGraph.should_recompute = true;
    }

}

void EditorState::setMusicSpeed(int newMusicSpeed) {
    musicSpeed = std::clamp(newMusicSpeed,1,20);
    if (music) {
        music->setPitch(musicSpeed/10.f);
    }
}

void EditorState::setMusicVolume(int newMusicVolume) {
    musicVolume = std::clamp(newMusicVolume,0,10);
    if (music) {
        Toolbox::updateVolume(*music,musicVolume);
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

    assert(editorState.chart.has_value());

    if (this->shouldRefreshValues) {

        shouldRefreshValues = false;

        difNamesInUse.clear();
        this->level = editorState.chart->ref.level;
        this->difficulty_name = editorState.chart->ref.dif_name;
        std::set<std::string> difNames{"BSC","ADV","EXT"};
        showCustomDifName = (difNames.find(difficulty_name) == difNames.end());

        for (auto const& tuple : editorState.fumen.Charts) {
            if (tuple.second != editorState.chart->ref) {
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
            if (difficulty_name.empty()) {
                comboPreview = "Choose One";
            } else {
                comboPreview = difficulty_name;
            }
        }
        if(ImGui::BeginCombo("Difficulty",comboPreview.c_str())) {
            for (auto dif_name : {"BSC","ADV","EXT"}) {
                if (difNamesInUse.find(dif_name) == difNamesInUse.end()) {
                    if(ImGui::Selectable(dif_name,dif_name == difficulty_name)) {
                        showCustomDifName = false;
                        difficulty_name = dif_name;
                    }
                } else {
                    ImGui::TextDisabled(dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom",&showCustomDifName)) {
                difficulty_name = "";
            }
            ImGui::EndCombo();
        }
        if (showCustomDifName) {
            Toolbox::InputTextColored(
                    difNamesInUse.find(difficulty_name) == difNamesInUse.end(),
                    "Chart name has to be unique",
                    "Difficulty Name",
                    &difficulty_name
            );
        }
        ImGui::InputInt("Level",&level);
        ImGui::Separator();
        if (difficulty_name.empty() or (difNamesInUse.find(difficulty_name) != difNamesInUse.end())) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Apply##New Chart");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Apply##New Chart")) {
                try {
                    Chart modified_chart = editorState.fumen.Charts.at(editorState.chart->ref.dif_name);
                    editorState.fumen.Charts.erase(editorState.chart->ref.dif_name);
                    modified_chart.dif_name = this->difficulty_name;
                    modified_chart.level = this->level;
                    if (not (editorState.fumen.Charts.emplace(modified_chart.dif_name,modified_chart)).second) {
                        throw std::runtime_error("Could not insert modified chart in fumen");
                    } else {
                        editorState.chart.emplace(editorState.fumen.Charts.at(modified_chart.dif_name));
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

EditorState::Chart_with_History::Chart_with_History(Chart &c) : ref(c) {
    history.push(std::make_shared<OpenChart>(c));
}
