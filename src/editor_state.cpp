#include "editor_state.hpp"

#include <SFML/System/Time.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <tinyfiledialogs.h>

#include "better_note.hpp"
#include "chart_state.hpp"
#include "history_actions.hpp"
#include "metadata_in_gui.hpp"
#include "time_interval.hpp"
#include "variant_visitor.hpp"

EditorState::EditorState(
    const better::Song& song_,
    const std::filesystem::path& assets_,
    const std::filesystem::path& song_path = {}
) : 
    song_path(song_path),
    playfield(assets_),
    linear_view(assets_),
    song(song_),
    metadata_in_gui(song_.metadata),
    applicable_timing(song.timing),
    assets(assets_)
{
    if (not song.charts.empty()) {
        open_chart(this->song.charts.begin()->second);
    }
    reload_music();
    reload_jacket();
};


const TimeInterval& EditorState::get_editable_range() {
    reload_editable_range();
    return editable_range;
}

void EditorState::set_playback_position(sf::Time newPosition) {
    newPosition = std::clamp(newPosition, editable_range.start, editable_range.end);
    previous_playback_position = newPosition - (sf::seconds(1) / 60.f);
    playback_position = newPosition;
    if (music_state) {
        if (
            playback_position >= sf::Time::Zero
            and playback_position < music_state->music.getDuration()
        ) {
            music_state->music.setPlayingOffset(playback_position);
        } else {
            music_state->music.stop();
        }
    }
}

Fraction EditorState::current_exact_beats() const {
    return applicable_timing.beats_at(playback_position);
};

Fraction EditorState::current_snaped_beats() const {
    const auto exact = current_exact_beats();
    return round_beats(exact, snap);
};

Fraction EditorState::beats_at(sf::Time time) const {
    return applicable_timing.beats_at(time);
};

sf::Time EditorState::time_at(Fraction beat) const {
    return applicable_timing.time_at(beat);
};

Fraction EditorState::get_snap_step() const {
    return Fraction{1, snap};
};

void EditorState::display_playfield(Marker& marker, MarkerEndingState markerEndingState) {
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(0, 0),
        ImVec2(FLT_MAX, FLT_MAX),
        Toolbox::CustomConstraints::ContentSquare
    );

    if (ImGui::Begin("Playfield", &showPlayfield, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        if (
            not ImGui::IsWindowHovered()
            and chart_state
            and chart_state->creating_long_note
        ) {
            // cancel long note creation if the mouse is or goes out of the playfield
            chart_state->long_note_being_created.reset();
            chart_state->creating_long_note = false;
        }

        float squareSize = ImGui::GetWindowSize().x / 4.f;
        float TitlebarHeight = ImGui::GetWindowSize().y - ImGui::GetWindowSize().x;
        int ImGuiIndex = 0;

        if (chart_state) {
            playfield.resize(static_cast<unsigned int>(ImGui::GetWindowSize().x));
            if (chart_state->long_note_being_created) {
                playfield.draw_tail_and_receptor(
                    make_long_note_dummy(
                        current_exact_beats(),
                        *chart_state->long_note_being_created
                    ),
                    playback_position,
                    applicable_timing
                );
            }

            auto display = VariantVisitor {
                [&, this](const better::TapNote& tap_note){
                    auto note_offset = (playback_position - this->time_at(tap_note.get_time()));
                    auto t = marker.getSprite(markerEndingState, note_offset.asSeconds());
                    if (t) {
                        ImGui::SetCursorPos({
                            tap_note.get_position().get_x() * squareSize,
                            TitlebarHeight + tap_note.get_position().get_y() * squareSize
                        });
                        ImGui::PushID(ImGuiIndex);
                        ImGui::Image(*t, {squareSize, squareSize});
                        ImGui::PopID();
                        ++ImGuiIndex;
                    }
                },
                [&, this](const better::LongNote& long_note){
                    this->playfield.draw_long_note(
                        long_note,
                        playback_position,
                        applicable_timing,
                        marker,
                        markerEndingState
                    );
                },
            };

            for (auto const& [_, note] : visibleNotes) {
                note.visit(display);
            }

            ImGui::SetCursorPos({0, TitlebarHeight});
            ImGui::Image(playfield.long_note.layer);
            ImGui::SetCursorPos({0, TitlebarHeight});
            ImGui::Image(playfield.marker_layer);
        }

        // Display button grid
        for (unsigned int y = 0; y < 4; ++y) {
            for (unsigned int x = 0; x < 4; ++x) {
                ImGui::PushID(x + 4 * y);
                ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0, 0, 0, 0));
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonHovered,
                    (ImVec4) ImColor::HSV(0, 0, 1.f, 0.1f));
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonActive,
                    (ImVec4) ImColor::HSV(0, 0, 1.f, 0.5f));
                if (ImGui::ImageButton(playfield.button, {squareSize, squareSize}, 0)) {
                    toggleNoteAtCurrentTime(x + 4 * y);
                }
                if (ImGui::IsItemHovered() and chart_state and chart_state->creating_long_note) {
                    // Deal with long note creation stuff
                    if (not chart_state->long_note_being_created) {
                        better::TapNote current_note{current_snaped_beats(), {x, y}};
                        chart_state->long_note_being_created.emplace(current_note, current_note);
                    } else {
                        chart_state->long_note_being_created->second = better::TapNote{
                            current_snaped_beats(), {x, y}
                        };
                    }
                }
                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
        }

        if (chart_state) {
            // Check for collisions then display them
            std::array<bool, 16> collisions = {};
            for (auto const& [_, note] : visibleNotes) {
                if (chart_state->chart.is_colliding(note)) {
                    collisions[note.get_position().index()] = true;
                }
            }
            for (int i = 0; i < 16; ++i) {
                if (collisions.at(i)) {
                    int x = i % 4;
                    int y = i / 4;
                    ImGui::SetCursorPos({x * squareSize, TitlebarHeight + y * squareSize});
                    ImGui::PushID(ImGuiIndex);
                    ImGui::Image(playfield.note_collision, {squareSize, squareSize});
                    ImGui::PopID();
                    ++ImGuiIndex;
                }
            }

            // Display selected notes
            for (auto const& [_, note] : visibleNotes) {
                if (chart_state->selected_notes.find(note) != chart_state->selected_notes.end()) {
                    ImGui::SetCursorPos({
                        note.get_position().get_x() * squareSize,
                        TitlebarHeight + note.get_position().get_y() * squareSize
                    });
                    ImGui::PushID(ImGuiIndex);
                    ImGui::Image(playfield.note_selected, {squareSize, squareSize});
                    ImGui::PopID();
                    ++ImGuiIndex;
                }
            }
        }
    }
    ImGui::End();
};

/*
Display all metadata in an editable form
*/
void EditorState::display_properties() {
    if (ImGui::Begin("Properties", &showProperties)) {
        if (ImGui::BeginChild("Album Cover", ImVec2(400, 0), true)) {
            if (jacket) {
                ImGui::Image(*jacket);
            }
        }
        ImGui::EndChild();

        ImGui::InputText("Title", &metadata_in_gui.title);
        ImGui::InputText("Artist", &metadata_in_gui.artist);

        if (Toolbox::InputTextColored(
            "Audio",
            &metadata_in_gui.audio,
            music_state.has_value(),
            "Invalid Audio Path"
        )) {
            reload_music();
        }
        if (Toolbox::InputTextColored(
            "Jacket",
            &metadata_in_gui.jacket,
            jacket.has_value(),
            "Invalid Jacket Path"
        )) {
            reload_jacket();
        }

        ImGui::Separator();
        
        ImGui::Text("Preview");
        ImGui::Checkbox("Use separate preview file", &metadata_in_gui.use_preview_file);
        if (metadata_in_gui.use_preview_file) {
            if (Toolbox::InputTextColored(
                "File",
                &metadata_in_gui.preview_file,
                preview_audio.has_value(),
                "Invalid Path"
            )) {
                reload_preview_audio();
            }
        } else {
            ImGui::Input
        }


    }
    ImGui::End();
};

/*
Display any information that would be useful for the user to troubleshoot the
status of the editor. Will appear in the "Editor Status" window
*/
void EditorState::display_status() {
    ImGui::Begin("Status", &showStatus, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (not music_state) {
            if (not metadata_in_gui.audio.empty()) {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "Invalid music path : %s",
                    metadata_in_gui.audio.c_str());
            } else {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "No music file loaded");
            }
        }

        if (not jacket) {
            if (not metadata_in_gui.jacket.empty()) {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "Invalid jacket path : %s",
                    metadata_in_gui.jacket.c_str());
            } else {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "No jacket loaded");
            }
        }
    }
    ImGui::End();
};

void EditorState::display_playback_status() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 25),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin(
        "Playback Status",
        &showPlaybackStatus,
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs
            | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (chart) {
            ImGui::Text("%s %d", chart->ref.dif_name.c_str(), chart->ref.level);
            ImGui::SameLine();
        } else {
            ImGui::TextDisabled("No chart selected");
            ImGui::SameLine();
        }
        ImGui::TextDisabled("Snap : ");
        ImGui::SameLine();
        ImGui::Text("%s", Toolbox::toOrdinal(snap * 4).c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.53, 0.53, 0.53, 1), "Beats :");
        ImGui::SameLine();
        ImGui::Text("%02.2f", this->get_current_beats());
        ImGui::SameLine();
        if (music) {
            ImGui::TextColored(
                ImVec4(0.53, 0.53, 0.53, 1),
                "Music File Offset :");
            ImGui::SameLine();
            ImGui::TextUnformatted(Toolbox::to_string(music->getPrecisePlayingOffset()).c_str());
            ImGui::SameLine();
        }
        ImGui::TextColored(ImVec4(0.53, 0.53, 0.53, 1), "Timeline Position :");
        ImGui::SameLine();
        ImGui::TextUnformatted(Toolbox::to_string(playback_position).c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
};

void EditorState::display_timeline() {
    ImGuiIO& io = ImGui::GetIO();

    float raw_height = io.DisplaySize.y * 0.9f;
    auto height = static_cast<int>(raw_height);

    if (
        chart.has_value()
        and (
            chart->density_graph.should_recompute
            or height != chart->density_graph.last_height.value_or(height)
        )
    ) {
        chart->density_graph.should_recompute = false;
        chart->density_graph.update(
            height,
            chart->ref,
            song.BPM,
            get_resolution()
        );
    }

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - 35, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize({45, height}, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0, 1.0, 1.1, 1.0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.240f, 0.520f, 0.880f, 0.500f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.240f, 0.520f, 0.880f, 0.700f));
    ImGui::Begin(
        "Timeline",
        &showTimeline,
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
    {
        if (chart) {
            ImGui::SetCursorPos({0, 0});
            ImGui::Image(chart->densityGraph.graph);
            AffineTransform<float> scroll(-song.offset, this->preview_end.asSeconds(), 1.f, 0.f);
            float slider_pos = scroll.transform(playbackPosition.asSeconds());
            ImGui::SetCursorPos({0, 0});
            if (ImGui::VSliderFloat("TimelineSlider", ImGui::GetContentRegionMax(), &slider_pos, 0.f, 1.f, "")) {
                setPlaybackAndMusicPosition(sf::seconds(scroll.backwards_transform(slider_pos)));
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(3);
};

void EditorState::display_chart_list() {
    if (ImGui::Begin("Chart List", &showChartList, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (this->song.Charts.empty()) {
            ImGui::Dummy({100, 0});
            ImGui::SameLine();
            ImGui::Text("- no charts -");
            ImGui::SameLine();
            ImGui::Dummy({100, 0});
        } else {
            ImGui::Dummy(ImVec2(300, 0));
            ImGui::Columns(3, "mycolumns");
            ImGui::TextDisabled("Difficulty");
            ImGui::NextColumn();
            ImGui::TextDisabled("Level");
            ImGui::NextColumn();
            ImGui::TextDisabled("Note Count");
            ImGui::NextColumn();
            ImGui::Separator();
            for (auto& tuple : song.Charts) {
                if (ImGui::Selectable(
                        tuple.first.c_str(),
                        chart ? chart->ref == tuple.second : false,
                        ImGuiSelectableFlags_SpanAllColumns)) {
                    ESHelper::save(*this);
                    chart.emplace(tuple.second, this->assets);
                }
                ImGui::NextColumn();
                ImGui::Text("%d", tuple.second.level);
                ImGui::NextColumn();
                ImGui::Text("%d", static_cast<int>(tuple.second.Notes.size()));
                ImGui::NextColumn();
                ImGui::PushID(&tuple);
                ImGui::PopID();
            }
        }
    }
    ImGui::End();
};

void EditorState::display_linear_view() {
    ImGui::SetNextWindowSize(ImVec2(204, 400), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(204, 204), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
    if (ImGui::Begin("Linear View", &showLinearView, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        if (chart) {
            linear_view.update(
                chart,
                playbackPosition,
                getCurrentTick(),
                song.BPM,
                getResolution(),
                ImGui::GetContentRegionMax());
            auto cursor_y = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            ImGui::SetCursorPos({0, cursor_y});
            ImGui::Image(linear_view.view);
        } else {
            ImGui::TextDisabled("- no chart selected -");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
};

saveChangesResponses EditorState::alertSaveChanges() {
    if (chart and (not chart->history.empty())) {
        int response = tinyfd_messageBox(
            "Warning",
            "Do you want to save changes ?",
            "yesnocancel",
            "warning",
            1);
        switch (response) {
            // cancel
            case 0:
                return saveChangesCancel;
            // yes
            case 1:
                return saveChangesYes;
            // no
            case 2:
                return saveChangesNo;
            default:
                std::stringstream ss;
                ss << "Got unexcpected result from tinyfd_messageBox : " << response;
                throw std::runtime_error(ss.str());
        }
    } else {
        return saveChangesDidNotDisplayDialog;
    }
};

/*
Saves if asked and returns false if user canceled
*/
bool EditorState::saveChangesOrCancel() {
    switch (alertSaveChanges()) {
        case saveChangesYes:
            ESHelper::save(*this);
        case saveChangesNo:
        case saveChangesDidNotDisplayDialog:
            return true;
        case saveChangesCancel:
        default:
            return false;
    }
};

/*
This SCREAAAAMS for optimisation, but in the meantime it works !
*/
void EditorState::updateVisibleNotes() {
    visibleNotes.clear();

    if (chart) {
        float position = playback_position.asSeconds();

        for (auto const& note : chart->ref.Notes) {
            float note_timing_in_seconds = getSecondsAt(note.getTiming());

            // we can leave early if the note is happening too far after the
            // position
            if (position > note_timing_in_seconds - 16.f / 30.f) {
                if (note.getLength() == 0) {
                    if (position < note_timing_in_seconds + 16.f / 30.f) {
                        visibleNotes.insert(note);
                    }
                } else {
                    float tail_end_in_seconds =
                        getSecondsAt(note.getTiming() + note.getLength());
                    if (position < tail_end_in_seconds + 16.f / 30.f) {
                        visibleNotes.insert(note);
                    }
                }
            }
        }
    }
};

/*
If a note is visible for the given pos, delete it
Otherwise create note at nearest tick
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
            toggledNotes.emplace(pos, static_cast<int>(roundf(getCurrentTick())));
            chart->ref.Notes.emplace(pos, static_cast<int>(roundf(getCurrentTick())));
        }

        chart->history.push(std::make_shared<ToggledNotes>(toggledNotes, not deleted_something));
        chart->density_graph.should_recompute = true;
    }
};

void EditorState::move_backwards_in_time() {
    auto beats = current_exact_beats();
    if (beats % get_snap_step() == 0) {
        beats -= get_snap_step();
    } else {
        beats -= beats % get_snap_step();
    }
    set_playback_position(applicable_timing.time_at(beats));
};

void EditorState::move_forwards_in_time() {
    auto beats = current_exact_beats();
    beats -= beats % get_snap_step();
    beats += get_snap_step();
    set_playback_position(applicable_timing.time_at(beats));
};

void EditorState::undo(NotificationsQueue& nq) {
    if (chart_state) {
        auto previous = chart_state->history.get_previous();
        if (previous) {
            nq.push(std::make_shared<UndoNotification>(**previous));
            (*previous)->undoAction(*this);
            chart_state->densityGraph.should_recompute = true;
        }
    }
};

void EditorState::redo(NotificationsQueue& nq) {
    if (chart_state) {
        auto next = chart_state->history.get_next();
        if (next) {
            nq.push(std::make_shared<RedoNotification>(**next));
            (*next)->doAction(*this);
            chart_state->densityGraph.should_recompute = true;
        }
    }
};

void EditorState::cut(NotificationsQueue& nq) {
    if (chart_state and (not chart_state->selectedNotes.empty())) {
        std::stringstream ss;
        ss << "Cut " << chart_state->selectedNotes.size() << " note";
        if (ed->chart->selectedNotes.size() > 1) {
            ss << "s";
        }
        nq.push(std::make_shared<TextNotification>(ss.str()));

        chart_state->notesClipboard.copy(ed->chart->selectedNotes);
        for (auto note : chart_state->selectedNotes) {
            chart_state->chart.Notes.erase(note);
        }
        chart_state->history.push(
            std::make_shared<RemoveNotes>(chart_state->selectedNotes)
        );
        chart_state->selectedNotes.clear();
    }
};

void EditorState::copy(NotificationsQueue& nq) {
    if (chart_state and (not chart_state->selectedNotes.empty())) {
        std::stringstream ss;
        ss << "Copied " << chart_state->selectedNotes.size() << " note";
        if (chart_state->selectedNotes.size() > 1) {
            ss << "s";
        }
        nq.push(std::make_shared<TextNotification>(ss.str()));
        chart_state->notesClipboard.copy(chart_state->selectedNotes);
    }
};

void EditorState::paste(NotificationsQueue& nq) {
    if (chart_state and (not chart_state->notesClipboard.empty())) {
        auto current_beat = current_snaped_beats();
        std::set<Note> pasted_notes = chart_state->notesClipboard.paste(current_beat);
        std::stringstream ss;
        ss << "Pasted " << pasted_notes.size() << " note";
        if (pasted_notes.size() > 1) {
            ss << "s";
        }
        nq.push(std::make_shared<TextNotification>(ss.str()));

        for (auto note : pasted_notes) {
            chart_state->chart.Notes.insert(note);
        }
        chart_state->selectedNotes = pasted_notes;
        chart_state->history.push(std::make_shared<AddNotes>(chart_state->selectedNotes));
        chart_state->densityGraph.should_recompute = true;
    }
};

void EditorState::delete_(NotificationsQueue& nq) {
    if (chart_state and (not chart_state->selectedNotes.empty())) {
        chart_state->history.push(
            std::make_shared<RemoveNotes>(chart_state->selectedNotes)
        );
        nq.push(
            std::make_shared<TextNotification>("Deleted selected notes")
        );
        for (auto note : chart_state->selectedNotes) {
            chart_state->chart.Notes.erase(note);
        }
        chart_state->selectedNotes.clear();
    }
}


void EditorState::reload_editable_range() {
    auto old_range = this->editable_range;
    TimeInterval new_range;
    if (music_state) {
        new_range += music_state->music.getDuration();
    }
    if (chart_state) {
        new_range += chart_state->chart.time_of_last_event().value_or(sf::Time::Zero);
    }

    new_range.end += sf::seconds(10);

    // If there is no music, make sure we can edit at least the first whole minute
    if (not music_state) {
        new_range += sf::seconds(60);
    }

    this->editable_range = new_range;
    if (old_range != new_range and this->chart_state.has_value()) {
        chart_state->density_graph.should_recompute = true;
    }
};

/*
 * Reloads the album cover from what's indicated in the "album cover path" field
 * of the song Resets the album cover state if anything fails
 */
void EditorState::reload_jacket() {
    if (not song_path.has_value() or not song.metadata.jacket.has_value()) {
        jacket.reset();
        return;
    }

    jacket.emplace();
    auto jacket_path = song_path->parent_path() / metadata_in_gui.jacket;

    if (
        not std::filesystem::exists(jacket_path)
        or not jacket->loadFromFile(jacket_path.string())
    ) {
        jacket.reset();
    }
};

/*
 * Reloads music from what's indicated in the "music path" field of the song
 * Resets the music state in case anything fails
 * Updates playbackPosition and preview_end as well
 */
void EditorState::reload_music() {
    if (not song_path.has_value()) {
        music_state.reset();
        return;
    }

    const auto absolute_music_path = song_path->parent_path() / metadata_in_gui.audio;
    try {
        music_state.emplace(absolute_music_path);
    } catch (const std::exception& e) {
        music_state.reset();
    }

    reload_editable_range();
    playback_position = std::clamp(
        playback_position,
        editable_range.start,
        editable_range.end
    );
    previous_playback_position = playback_position;
};

void EditorState::reload_preview_audio() {
    if (not song_path.has_value()) {
        preview_audio.reset();
        return;
    }

    const auto path = song_path->parent_path() / metadata_in_gui.preview_file;
    try {
        preview_audio.emplace(path);
    } catch (const std::exception& e) {
        preview_audio.reset();
    }
};

void reload_applicable_timing() {
    // TODO: implement
}

void EditorState::open_chart(better::Chart& chart) {
    chart_state.emplace(chart, assets);
    reload_applicable_timing();
    reload_editable_range();
};

void ESHelper::save(EditorState& ed) {
    try {
        ed.song.autoSaveAsMemon();
    } catch (const std::exception& e) {
        tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
    }
}

void ESHelper::open(std::optional<EditorState>& ed, std::filesystem::path assets, std::filesystem::path settings) {
    const char* _filepath =
        tinyfd_openFileDialog("Open File", nullptr, 0, nullptr, nullptr, false);
    if (_filepath != nullptr) {
        auto filepath = std::filesystem::path{_filepath};
        ESHelper::openFromFile(ed, filepath, assets, settings);
    }
}

void ESHelper::openFromFile(
    std::optional<EditorState>& ed,
    std::filesystem::path file,
    std::filesystem::path assets,
    std::filesystem::path settings
) {
    try {
        Fumen f(file);
        f.autoLoadFromMemon();
        ed.emplace(f, assets);
        Toolbox::pushNewRecentFile(std::filesystem::canonical(ed->song.path), settings);
    } catch (const std::exception& e) {
        tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
    }
}

/*
 * returns true if user saved or if saving wasn't necessary
 * returns false if user canceled
 */
bool ESHelper::saveOrCancel(std::optional<EditorState>& ed) {
    if (ed) {
        return ed->saveChangesOrCancel();
    } else {
        return true;
    }
}

/*
 * Returns the newly created chart if there is one
 */
std::optional<Chart> ESHelper::NewChartDialog::display(EditorState& editorState) {
    std::optional<Chart> newChart;
    if (ImGui::Begin(
            "New Chart",
            &editorState.showNewChartDialog,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
        if (showCustomDifName) {
            comboPreview = "Custom";
        } else {
            if (difficulty.empty()) {
                comboPreview = "Choose One";
            } else {
                comboPreview = difficulty;
            }
        }
        if (ImGui::BeginCombo("Difficulty", comboPreview.c_str())) {
            for (auto dif_name : {"BSC", "ADV", "EXT"}) {
                if (editorState.song.Charts.find(dif_name)
                    == editorState.song.Charts.end()) {
                    if (ImGui::Selectable(dif_name, dif_name == difficulty)) {
                        showCustomDifName = false;
                        difficulty = dif_name;
                    }
                } else {
                    ImGui::TextDisabled("%s", dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom", &showCustomDifName)) {
                difficulty = "";
            }
            ImGui::EndCombo();
        }
        if (showCustomDifName) {
            Toolbox::InputTextColored(
                editorState.song.Charts.find(difficulty)
                    == editorState.song.Charts.end(),
                "Chart name has to be unique",
                "Difficulty Name",
                &difficulty);
        }
        ImGui::InputInt("Level", &level);
        ImGui::Separator();
        if (ImGui::TreeNode("Advanced##New Chart")) {
            ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
            if (ImGui::InputInt("Resolution", &resolution)) {
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
                ImGui::BulletText(
                    "Leave the default unless you know what you're doing");
                ImGui::EndTooltip();
            }
            ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
            ImGui::TreePop();
        }
        ImGui::Separator();
        if (difficulty.empty()
            or (editorState.song.Charts.find(difficulty)
                != editorState.song.Charts.end())) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Create Chart##New Chart");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Create Chart##New Chart")) {
                try {
                    newChart.emplace(difficulty, level, resolution);
                } catch (const std::exception& e) {
                    tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                }
            }
        }
    }
    ImGui::End();
    return newChart;
}

void ESHelper::ChartPropertiesDialog::display(EditorState& editorState, std::filesystem::path assets) {
    assert(editorState.chart.has_value());

    if (this->shouldRefreshValues) {
        shouldRefreshValues = false;

        difNamesInUse.clear();
        this->level = editorState.chart->ref.level;
        this->difficulty_name = editorState.chart->ref.dif_name;
        std::set<std::string> difNames {"BSC", "ADV", "EXT"};
        showCustomDifName = (difNames.find(difficulty_name) == difNames.end());

        for (auto const& tuple : editorState.song.Charts) {
            if (tuple.second != editorState.chart->ref) {
                difNamesInUse.insert(tuple.first);
            }
        }
    }

    if (ImGui::Begin(
            "Chart Properties",
            &editorState.showChartProperties,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
        if (showCustomDifName) {
            comboPreview = "Custom";
        } else {
            if (difficulty_name.empty()) {
                comboPreview = "Choose One";
            } else {
                comboPreview = difficulty_name;
            }
        }
        if (ImGui::BeginCombo("Difficulty", comboPreview.c_str())) {
            for (auto dif_name : {"BSC", "ADV", "EXT"}) {
                if (difNamesInUse.find(dif_name) == difNamesInUse.end()) {
                    if (ImGui::Selectable(dif_name, dif_name == difficulty_name)) {
                        showCustomDifName = false;
                        difficulty_name = dif_name;
                    }
                } else {
                    ImGui::TextDisabled("%s", dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom", &showCustomDifName)) {
                difficulty_name = "";
            }
            ImGui::EndCombo();
        }
        if (showCustomDifName) {
            Toolbox::InputTextColored(
                difNamesInUse.find(difficulty_name) == difNamesInUse.end(),
                "Chart name has to be unique",
                "Difficulty Name",
                &difficulty_name);
        }
        ImGui::InputInt("Level", &level);
        ImGui::Separator();
        if (difficulty_name.empty()
            or (difNamesInUse.find(difficulty_name) != difNamesInUse.end())) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Apply##New Chart");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Apply##New Chart")) {
                try {
                    Chart modified_chart =
                        editorState.song.Charts.at(editorState.chart->ref.dif_name);
                    editorState.song.Charts.erase(editorState.chart->ref.dif_name);
                    modified_chart.dif_name = this->difficulty_name;
                    modified_chart.level = this->level;
                    if (not(editorState.song.Charts.emplace(modified_chart.dif_name, modified_chart))
                               .second) {
                        throw std::runtime_error(
                            "Could not insert modified chart in song");
                    } else {
                        editorState.chart.emplace(
                            editorState.song.Charts.at(modified_chart.dif_name),
                            assets
                        );
                        shouldRefreshValues = true;
                    }
                } catch (const std::exception& e) {
                    tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                }
            }
        }
    }
    ImGui::End();
}
