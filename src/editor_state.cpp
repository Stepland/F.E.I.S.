#include "editor_state.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>

#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <nowide/fstream.hpp>
#include <sstream>
#include <SFML/System/Time.hpp>
#include <stdexcept>
#include <string>
#include <tinyfiledialogs.h>

#include "better_note.hpp"
#include "chart_state.hpp"
#include "file_dialogs.hpp"
#include "history_actions.hpp"
#include "imgui_extras.hpp"
#include "json_decimal_parser.hpp"
#include "special_numeric_types.hpp"
#include "src/better_song.hpp"
#include "src/chart.hpp"
#include "std_optional_extras.hpp"
#include "variant_visitor.hpp"

EditorState::EditorState(const std::filesystem::path& assets_) : 
    playfield(assets_),
    linear_view(assets_),
    applicable_timing(song.timing),
    assets(assets_)
{
    reload_music();
    reload_jacket();
};

EditorState::EditorState(
    const better::Song& song_,
    const std::filesystem::path& assets_,
    const std::filesystem::path& song_path = {}
) : 
    song(song_),
    song_path(song_path),
    playfield(assets_),
    linear_view(assets_),
    applicable_timing(song.timing),
    assets(assets_)
{
    if (not song.charts.empty()) {
        auto& [name, _] = *this->song.charts.begin();
        open_chart(name);
    }
    reload_music();
    reload_jacket();
};


const Interval<sf::Time>& EditorState::get_editable_range() {
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

            for (auto const& note : chart_state->visible_notes) {
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
                    if (chart_state) {
                        chart_state->toggle_note(playback_position, snap, {x, y});
                    }
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
            for (auto const& note : chart_state->visible_notes) {
                if (chart_state->chart.notes.is_colliding(note, applicable_timing)) {
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
            for (auto const& note : chart_state->visible_notes) {
                if (chart_state->selected_notes.contains(note)) {
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

        ImGui::InputText("Title", &song.metadata.title);
        ImGui::InputText("Artist", &song.metadata.artist);

        if (feis::InputTextColored(
            "Audio",
            &song.metadata.audio,
            music_state.has_value(),
            "Invalid Audio Path"
        )) {
            reload_music();
        }
        if (feis::InputTextColored(
            "Jacket",
            &song.metadata.jacket,
            jacket.has_value(),
            "Invalid Jacket Path"
        )) {
            reload_jacket();
        }

        ImGui::Separator();
        
        ImGui::Text("Preview");
        ImGui::Checkbox("Use separate preview file", &song.metadata.use_preview_file);
        if (song.metadata.use_preview_file) {
            if (feis::InputTextColored(
                "File",
                &song.metadata.preview_file,
                preview_audio.has_value(),
                "Invalid Path"
            )) {
                reload_preview_audio();
            }
        } else {
            if (feis::InputDecimal("Start", &song.metadata.preview_loop.start)) {
                song.metadata.preview_loop.start = std::max(
                    Decimal{0},
                    song.metadata.preview_loop.start
                );
                if (music_state.has_value()) {
                    song.metadata.preview_loop.start = std::min(
                        Decimal{music_state->music.getDuration().asMicroseconds()} / 1000000,
                        song.metadata.preview_loop.start
                    );
                }
            }
            if (feis::InputDecimal("Duration", &song.metadata.preview_loop.duration)) {
                song.metadata.preview_loop.duration = std::max(
                    Decimal{0},
                    song.metadata.preview_loop.duration
                );
                if (music_state.has_value()) {
                    song.metadata.preview_loop.start = std::min(
                        (
                            Decimal{
                                music_state->music
                                .getDuration()
                                .asMicroseconds()
                            } / 1000000 
                            - song.metadata.preview_loop.start
                        ),
                        song.metadata.preview_loop.start
                    );
                }
            }
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
            if (not song.metadata.audio.empty()) {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "Invalid music path : %s",
                    song.metadata.audio.c_str());
            } else {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "No music file loaded");
            }
        }

        if (not jacket) {
            if (not song.metadata.jacket.empty()) {
                ImGui::TextColored(
                    ImVec4(1, 0.42, 0.41, 1),
                    "Invalid jacket path : %s",
                    song.metadata.jacket.c_str());
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
        if (chart_state) {
            ImGui::TextUnformatted(
                fmt::format(
                    "{} {}",
                    chart_state->difficulty_name,
                    better::stringify_level(chart_state->chart.level)
                ).c_str()
            );
            ImGui::SameLine();
        } else {
            ImGui::TextDisabled("No chart selected");
            ImGui::SameLine();
        }
        ImGui::TextDisabled("Snap :");
        ImGui::SameLine();
        ImGui::Text("%s", Toolbox::toOrdinal(snap * 4).c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("Beats :");
        ImGui::SameLine();
        ImGui::TextUnformatted(convert_to_decimal(this->current_exact_beats(), 3).format(".3f").c_str());
        ImGui::SameLine();
        if (music_state) {
            ImGui::TextDisabled("Music File Offset :");
            ImGui::SameLine();
            ImGui::TextUnformatted(Toolbox::to_string(music_state->music.getPrecisePlayingOffset()).c_str());
            ImGui::SameLine();
        }
        ImGui::TextDisabled("Timeline Position :");
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
        chart_state.has_value()
        and (
            chart_state->density_graph.should_recompute
            or height != chart_state->density_graph.last_height.value_or(height)
        )
    ) {
        chart_state->density_graph.should_recompute = false;
        chart_state->density_graph.update(
            height,
            chart_state->chart,
            editable_range.start,
            editable_range.end
        );
    }

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - 35, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize({45.f, static_cast<float>(height)}, ImGuiCond_Always);
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
        if (chart_state) {
            ImGui::SetCursorPos({0, 0});
            ImGui::Image(chart_state->density_graph.graph);
            // The output is reversed because we are repurposing a vertical
            // slider, which goes from 0 AT THE BOTTOM to 1 AT THE TOP, which is
            // the opposite of what we want
            AffineTransform<float> scroll(
                editable_range.start.asSeconds(),
                editable_range.end.asSeconds(),
                1.f,
                0.f
            );
            float slider_pos = scroll.transform(playback_position.asSeconds());
            ImGui::SetCursorPos({0, 0});
            if (ImGui::VSliderFloat("TimelineSlider", ImGui::GetContentRegionMax(), &slider_pos, 0.f, 1.f, "")) {
                set_playback_position(sf::seconds(scroll.backwards_transform(slider_pos)));
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(3);
};

void EditorState::display_chart_list() {
    if (ImGui::Begin("Chart List", &showChartList, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (this->song.charts.empty()) {
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
            for (auto& [name, chart] : song.charts) {
                if (ImGui::Selectable(
                    name.c_str(),
                    chart_state ? chart_state->difficulty_name == name : false,
                    ImGuiSelectableFlags_SpanAllColumns
                )) {
                    open_chart(name);
                }
                ImGui::NextColumn();
                ImGui::TextUnformatted(better::stringify_level(chart.level).c_str());
                ImGui::NextColumn();
                ImGui::Text("%d", static_cast<int>(chart.notes.size()));
                ImGui::NextColumn();
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
        if (chart_state) {
            linear_view.update(
                *chart_state,
                playback_position,
                ImGui::GetContentRegionMax()
            );
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

bool EditorState::needs_to_save() const {
    if (chart_state) {
        return not chart_state->history.current_state_is_saved();
    } else {
        return false;
    }
}

EditorState::UserWantsToSave EditorState::ask_if_user_wants_to_save() const {
    int response_code = tinyfd_messageBox(
        "Warning",
        "Do you want to save changes ?",
        "yesnocancel",
        "warning",
        1
    );
    switch (response_code) {
        // cancel
        case 0:
            return EditorState::UserWantsToSave::Cancel;
        // yes
        case 1:
            return EditorState::UserWantsToSave::Yes;
        // no
        case 2:
            return EditorState::UserWantsToSave::No;
        default:
            throw std::runtime_error(fmt::format(
                "Got unexcpected response code from tinyfd_messageBox : {}",
                response_code
            ));
    }
};

EditorState::SaveOutcome EditorState::save_if_needed() {
    if (not needs_to_save()) {
        return EditorState::SaveOutcome::NoSavingNeeded;
    }
    switch (ask_if_user_wants_to_save()) {
        case EditorState::UserWantsToSave::Yes:
            {
                const auto path = ask_for_save_path_if_needed();
                if (not path) {
                    return EditorState::SaveOutcome::UserCanceled; 
                } else {
                    save(*path);
                    return EditorState::SaveOutcome::UserSaved;
                }
            }
        case EditorState::UserWantsToSave::No:
            return EditorState::SaveOutcome::UserDeclindedSaving;
        case EditorState::UserWantsToSave::Cancel:
            return EditorState::SaveOutcome::UserCanceled;
    }
}

std::optional<std::filesystem::path> EditorState::ask_for_save_path_if_needed() {
    if (song_path) {
        return song_path;
    } else {
        return feis::ask_for_save_path();
    }
}

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
        auto previous = chart_state->history.pop_previous();
        if (previous) {
            nq.push(std::make_shared<UndoNotification>(**previous));
            (*previous)->undoAction(*this);
            chart_state->density_graph.should_recompute = true;
        }
    }
};

void EditorState::redo(NotificationsQueue& nq) {
    if (chart_state) {
        auto next = chart_state->history.pop_next();
        if (next) {
            nq.push(std::make_shared<RedoNotification>(**next));
            (*next)->doAction(*this);
            chart_state->density_graph.should_recompute = true;
        }
    }
};

void EditorState::open_chart(const std::string& name) {
    auto& [name_ref, chart] = *song.charts.find(name);
    chart_state.emplace(chart, name_ref, assets);
    reload_editable_range();
    reload_applicable_timing();
};


void EditorState::reload_editable_range() {
    auto old_range = this->editable_range;
    Interval<sf::Time> new_range;
    if (music_state) {
        new_range += music_state->music.getDuration();
    }
    if (chart_state and not chart_state->chart.notes.empty()) {
        const auto beat_of_last_event = chart_state->chart.notes.crbegin()->second.get_end();
        new_range += applicable_timing.time_at(beat_of_last_event);
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
    if (not song_path.has_value() or song.metadata.jacket.empty()) {
        jacket.reset();
        return;
    }

    jacket.emplace();
    auto jacket_path = song_path->parent_path() / song.metadata.jacket;

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
    if (not song_path.has_value() or song.metadata.audio.empty()) {
        music_state.reset();
        return;
    }

    const auto absolute_music_path = song_path->parent_path() / song.metadata.audio;
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
    if (not song_path.has_value() or song.metadata.preview_file.empty()) {
        preview_audio.reset();
        return;
    }

    const auto path = song_path->parent_path() / song.metadata.preview_file;
    preview_audio.emplace();
    if (not preview_audio->openFromFile(path.string())) {
        preview_audio.reset();
    }
};

void EditorState::reload_applicable_timing() {
    if (chart_state and chart_state->chart.timing) {
        applicable_timing = *chart_state->chart.timing;
    } else {
        applicable_timing = song.timing;
    }
}

void EditorState::save(const std::filesystem::path& path) {
    const auto memon = song.dump_to_memon_1_0_0();
    nowide::ofstream file{path};
    if (not file) {
        throw std::runtime_error(
            fmt::format("Cannot write to file {}", path.string())
        );
    }
    file << memon.dump(4) << std::endl;
    file.close();
    if (not file) {
        throw std::runtime_error(
            fmt::format("Error while closing file {}", path.string())
        );
    }
    if (chart_state) {
        chart_state->history.mark_as_saved();
    }
}

void feis::open(std::optional<EditorState>& ed, std::filesystem::path assets, std::filesystem::path settings) {
    const char* _filepath = tinyfd_openFileDialog(
        "Open File", nullptr, 0, nullptr, nullptr, false
    );
    // convert to u8string so std::filesystem::path works correctly on windows
    const auto filepath_u8string = std::u8string{_filepath, _filepath+std::strlen(_filepath)};
    if (_filepath != nullptr) {
        auto filepath = std::filesystem::path{filepath_u8string};
        feis::open_from_file(ed, filepath, assets, settings);
    }
}

void feis::open_from_file(
    std::optional<EditorState>& ed,
    std::filesystem::path song_path,
    std::filesystem::path assets,
    std::filesystem::path settings
) {
    try {
        // force utf-8 on windows
        nowide::ifstream f{song_path.string()};
        if (not f) {
            tinyfd_messageBox(
                "Error",
                fmt::format("Could not open file {}", song_path.string()).c_str(),
                "ok",
                "error",
                1
            );
            return;
        };
        const auto json = parse_decimal_json(f);
        auto song = better::Song::load_from_memon(json);
        ed.emplace(song, assets, song_path);
        Toolbox::pushNewRecentFile(std::filesystem::canonical(song_path), settings);
    } catch (const std::exception& e) {
        tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
    }
}

/*
 * Returns the newly created chart if there is one
 */
std::optional<std::pair<std::string, better::Chart>> feis::NewChartDialog::display(EditorState& editorState) {
    if (ImGui::Begin(
            "New Chart",
            &editorState.showNewChartDialog,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
        if (show_custom_dif_name) {
            combo_preview = "Custom";
        } else {
            if (difficulty.empty()) {
                combo_preview = "Choose One";
            } else {
                combo_preview = difficulty;
            }
        }
        if (ImGui::BeginCombo("Difficulty", combo_preview.c_str())) {
            for (auto dif_name : {"BSC", "ADV", "EXT"}) {
                if (not editorState.song.charts.contains(dif_name)) {
                    if (ImGui::Selectable(dif_name, dif_name == difficulty)) {
                        show_custom_dif_name = false;
                        difficulty = dif_name;
                    }
                } else {
                    ImGui::TextDisabled("%s", dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom", &show_custom_dif_name)) {
                difficulty = "";
            }
            ImGui::EndCombo();
        }
        if (show_custom_dif_name) {
            feis::InputTextColored(
                "Difficulty Name",
                &difficulty,
                not editorState.song.charts.contains(difficulty),
                "Chart name has to be unique"
            );
        }
        feis::InputDecimal("Level", &level);
        ImGui::Separator();
        ImGui::BeginDisabled(difficulty.empty() or (editorState.song.charts.contains(difficulty)));
        if (ImGui::Button("Create Chart##New Chart")) {
            ImGui::EndDisabled();
            ImGui::End();
            try {
                return {{difficulty, {.level = level}}};
            } catch (const std::exception& e) {
                tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                return {};
            }
        };
        ImGui::EndDisabled();
    }
    ImGui::End();
    return {};
}

void feis::ChartPropertiesDialog::display(EditorState& editor_state) {
    assert(editor_state.chart_state.has_value());

    if (this->should_refresh_values) {
        should_refresh_values = false;

        difficulty_names_in_use.clear();
        this->level = editor_state.chart_state->chart.level.value_or(0);
        this->difficulty_name = editor_state.chart_state->difficulty_name;
        this->show_custom_dif_name = (
            difficulty_name == "BSC"
            or difficulty_name == "ADV"
            or difficulty_name == "EXT"
        );

        for (auto const& [name, _] : editor_state.song.charts) {
            if (name != editor_state.chart_state->difficulty_name) {
                difficulty_names_in_use.insert(name);
            }
        }
    }

    if (ImGui::Begin(
            "Chart Properties",
            &editor_state.showChartProperties,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
        if (show_custom_dif_name) {
            combo_preview = "Custom";
        } else {
            if (difficulty_name.empty()) {
                combo_preview = "Choose One";
            } else {
                combo_preview = difficulty_name;
            }
        }
        if (ImGui::BeginCombo("Difficulty", combo_preview.c_str())) {
            for (auto dif_name : {"BSC", "ADV", "EXT"}) {
                if (not difficulty_names_in_use.contains(dif_name)) {
                    if (ImGui::Selectable(dif_name, dif_name == difficulty_name)) {
                        show_custom_dif_name = false;
                        difficulty_name = dif_name;
                    }
                } else {
                    ImGui::TextDisabled("%s", dif_name);
                }
            }
            ImGui::Separator();
            if (ImGui::Selectable("Custom", &show_custom_dif_name)) {
                difficulty_name = "";
            }
            ImGui::EndCombo();
        }
        if (show_custom_dif_name) {
            feis::InputTextColored(
                "Difficulty Name",
                &difficulty_name,
                not difficulty_names_in_use.contains(difficulty_name),
                "Chart name has to be unique"
            );
        }
        feis::InputDecimal("Level", &level);
        ImGui::Separator();
        if (difficulty_name.empty()
            or (difficulty_names_in_use.find(difficulty_name) != difficulty_names_in_use.end())) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Apply##New Chart");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Apply##New Chart")) {
                try {
                    auto modified_chart = editor_state.song.charts.extract(editor_state.chart_state->difficulty_name);
                    modified_chart.key() = this->difficulty_name;
                    modified_chart.mapped().level = this->level;
                    const auto [_1, inserted, _2] = editor_state.song.charts.insert(std::move(modified_chart));
                    if (not inserted) {
                        throw std::runtime_error("Could not insert modified chart in song");
                    } else {
                        editor_state.open_chart(this->difficulty_name);
                        should_refresh_values = true;
                    }
                } catch (const std::exception& e) {
                    tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                }
            }
        }
    }
    ImGui::End();
}
