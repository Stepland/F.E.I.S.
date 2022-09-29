#include "editor_state.hpp"

#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Audio/SoundStream.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>

#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <memory>
#include <nowide/fstream.hpp>
#include <sstream>
#include <SFML/System/Time.hpp>
#include <stdexcept>
#include <string>
#include <tinyfiledialogs.h>
#include <variant>

#include "better_note.hpp"
#include "better_song.hpp"
#include "chart_state.hpp"
#include "file_dialogs.hpp"
#include "history_item.hpp"
#include "imgui_extras.hpp"
#include "json_decimal_handling.hpp"
#include "long_note_dummy.hpp"
#include "notifications_queue.hpp"
#include "special_numeric_types.hpp"
#include "src/custom_sfml_audio/synced_sound_streams.hpp"
#include "variant_visitor.hpp"

EditorState::EditorState(const std::filesystem::path& assets_) : 
    note_claps(std::make_shared<NoteClaps>(nullptr, nullptr, assets_, 1.f)),
    beat_ticks(std::make_shared<BeatTicks>(nullptr, assets_, 1.f)),
    playfield(assets_),
    linear_view(assets_),
    applicable_timing(song.timing),
    assets(assets_)
{
    reload_music();
    reload_jacket();
    audio.add_stream(note_clap_stream, {note_claps, true});
};

EditorState::EditorState(
    const better::Song& song_,
    const std::filesystem::path& assets_,
    const std::filesystem::path& song_path = {}
) : 
    song(song_),
    song_path(song_path),
    note_claps(std::make_shared<NoteClaps>(nullptr, nullptr, assets_, 1.f)),
    beat_ticks(std::make_shared<BeatTicks>(nullptr, assets_, 1.f)),
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
    audio.add_stream(note_clap_stream, {note_claps, true});
};

int EditorState::get_volume() const {
    return volume;
}

void EditorState::set_volume(int newMusicVolume) {
    volume = std::clamp(newMusicVolume, 0, 10);
    if (music.has_value()) {
        (**music).setVolume(Toolbox::convertVolumeToNormalizedDB(volume)*100.f);
    }
}

void EditorState::volume_up() {
    set_volume(volume + 1);
}

void EditorState::volume_down() {
    set_volume(volume - 1);
}

int EditorState::get_speed() const {
    return speed;
}

void EditorState::set_speed(int newMusicSpeed) {
    speed = std::clamp(newMusicSpeed, 1, 20);
    set_pitch(speed / 10.f);
}

void EditorState::speed_up() {
    set_speed(speed + 1);
}

void EditorState::speed_down() {
    set_speed(speed - 1);
}


const Interval<sf::Time>& EditorState::get_editable_range() {
    reload_editable_range();
    return editable_range;
};

void EditorState::toggle_playback() {
    if (get_status() != sf::SoundSource::Playing) {
        play();
    } else {
        pause();
    }
}

void EditorState::toggle_beat_ticks() {
    if (audio.contains_stream(beat_tick_stream)) {
        audio.remove_stream(beat_tick_stream);
    } else {
        audio.add_stream(beat_tick_stream, {beat_ticks, true});
    }
}

void EditorState::play() {
    audio.play();
}

void EditorState::pause() {
    audio.pause();
}

void EditorState::stop() {
    audio.stop();
}

sf::SoundSource::Status EditorState::get_status() {
    return audio.getStatus();
}

void EditorState::set_pitch(float pitch) {
    std::map<std::string, NewStream> update;
    if (audio.contains_stream(note_clap_stream)) {
        note_claps = note_claps->with_pitch(pitch);
        update[note_clap_stream] = {note_claps, true};
    }
    if (audio.contains_stream(beat_tick_stream)) {
        beat_ticks = beat_ticks->with_pitch(pitch);
        update[beat_tick_stream] = {beat_ticks, true};
    }
    audio.update_streams(update);
    audio.setPitch(pitch);
}

void EditorState::set_playback_position(std::variant<sf::Time, Fraction> newPosition) {
    const auto clamp_ = VariantVisitor {
        [this](const sf::Time& seconds) {
            return std::variant<sf::Time, Fraction>(
                std::clamp(
                    seconds,
                    this->editable_range.start,
                    this->editable_range.end
                )
            );
        },
        [this](const Fraction& beats) {
            return std::variant<sf::Time, Fraction>(
                std::clamp(
                    beats,
                    this->beats_at(this->editable_range.start),
                    this->beats_at(this->editable_range.end)
                )
            );
        },
    };
    newPosition = std::visit(clamp_, newPosition);
    previous_playback_position = playback_position;
    playback_position = newPosition;
    const auto now = current_time();
    if (now >= sf::Time::Zero and now < editable_range.end) {
        audio.setPlayingOffset(now);
    } else {
        stop();
    }
};

sf::Time EditorState::get_precise_playback_position() {
    return audio.getPrecisePlayingOffset();
}

Fraction EditorState::current_exact_beats() const {
    const auto current_exact_beats_ = VariantVisitor {
        [this](const sf::Time& seconds) { return this->beats_at(seconds); },
        [](const Fraction& beats) { return beats; },
    };
    return std::visit(current_exact_beats_, playback_position);
};

Fraction EditorState::current_snaped_beats() const {
    const auto exact = current_exact_beats();
    return round_beats(exact, snap);
};

Fraction EditorState::previous_exact_beats() const {
    const auto current_exact_beats_ = VariantVisitor {
        [this](const sf::Time& seconds) { return this->beats_at(seconds); },
        [](const Fraction& beats) { return beats; },
    };
    return std::visit(current_exact_beats_, previous_playback_position);
}

sf::Time EditorState::current_time() const {
    const auto current_time_ = VariantVisitor {
        [](const sf::Time& seconds) { return seconds; },
        [this](const Fraction& beats) { return this->time_at(beats); },
    };
    return std::visit(current_time_, playback_position);
}

sf::Time EditorState::previous_time() const {
    const auto current_time_ = VariantVisitor {
        [](const sf::Time& seconds) { return seconds; },
        [this](const Fraction& beats) { return this->time_at(beats); },
    };
    return std::visit(current_time_, previous_playback_position);
}

Fraction EditorState::beats_at(sf::Time time) const {
    return applicable_timing.beats_at(time);
};

sf::Time EditorState::time_at(Fraction beat) const {
    return applicable_timing.time_at(beat);
};

Fraction EditorState::get_snap_step() const {
    return Fraction{1, snap};
};

void EditorState::display_playfield(Marker& marker, Judgement markerEndingState) {
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
                    make_playfield_long_note_dummy(
                        current_exact_beats(),
                        *chart_state->long_note_being_created,
                        get_snap_step()
                    ),
                    current_time(),
                    applicable_timing
                );
            }

            auto display = VariantVisitor {
                [&, this](const better::TapNote& tap_note){
                    auto note_offset = (this->current_time() - this->time_at(tap_note.get_time()));
                    auto t = marker.at(markerEndingState, note_offset);
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
                        current_time(),
                        applicable_timing,
                        marker,
                        markerEndingState
                    );
                },
            };

            for (const auto& [_, note] : chart_state->visible_notes) {
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
                        chart_state->toggle_note(
                            current_time(),
                            snap, 
                            {x, y},
                            applicable_timing
                        );
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
            for (const auto& [_, note] : chart_state->visible_notes) {
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
            for (const auto& [_, note] : chart_state->visible_notes) {
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
    if (ImGui::Begin(
        "Properties",
        &showProperties,
        ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_AlwaysAutoResize
    )) {
        if (jacket) {
            if (jacket) {
                ImGui::Image(*jacket, sf::Vector2f(300, 300));
            }
        } else {
            ImGui::BeginChild("Album Cover", ImVec2(300, 300), true, ImGuiWindowFlags_NoResize);
            ImGui::EndChild();
        }

        ImGui::InputText("Title", &song.metadata.title);
        ImGui::InputText("Artist", &song.metadata.artist);

        if (feis::InputTextColored(
            "Audio",
            &song.metadata.audio,
            music.has_value(),
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
                if (music.has_value()) {
                    song.metadata.preview_loop.start = std::min(
                        Decimal{(**music).getDuration().asMicroseconds()} / 1000000,
                        song.metadata.preview_loop.start
                    );
                }
            }
            if (feis::InputDecimal("Duration", &song.metadata.preview_loop.duration)) {
                song.metadata.preview_loop.duration = std::max(
                    Decimal{0},
                    song.metadata.preview_loop.duration
                );
                if (music.has_value()) {
                    song.metadata.preview_loop.start = std::min(
                        (
                            Decimal{
                                (**music)
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
        if (not music.has_value()) {
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
        ImGui::TextUnformatted(fmt::format("{:.3f}", static_cast<double>(current_exact_beats())).c_str());
        ImGui::SameLine();
        if (music.has_value()) {
            ImGui::TextDisabled("Music File Offset :");
            ImGui::SameLine();
            ImGui::TextUnformatted(Toolbox::to_string(audio.getPlayingOffset()).c_str());
            ImGui::SameLine();
        }
        ImGui::TextDisabled("Timeline Position :");
        ImGui::SameLine();
        ImGui::TextUnformatted(Toolbox::to_string(current_time()).c_str());
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
            applicable_timing,
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
            float slider_pos = scroll.transform(current_time().asSeconds());
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
    ImGui::SetNextWindowSize(ImVec2(304, 500), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(304, 304), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
    if (ImGui::Begin("Linear View", &showLinearView, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        if (chart_state) {
            auto header_height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            ImGui::SetCursorPos({0, header_height});
            linear_view.draw(
                ImGui::GetWindowDrawList(),
                *chart_state,
                applicable_timing,
                current_exact_beats(),
                beats_at(editable_range.end),
                get_snap_step(),
                ImGui::GetContentRegionMax(),
                ImGui::GetCursorScreenPos()
            );
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
};

EditorState::UserWantsToSave EditorState::ask_if_user_wants_to_save() const {
    int response_code = tinyfd_messageBox(
        "Warning",
        "Chart has unsaved changes, do you want to save ?",
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

EditorState::SaveOutcome EditorState::save_if_needed_and_user_wants_to() {
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
        default:
            return EditorState::SaveOutcome::UserCanceled;
    }
};

EditorState::SaveOutcome EditorState::save_if_needed() {
    if (not needs_to_save()) {
        return EditorState::SaveOutcome::NoSavingNeeded;
    } else {
        const auto path = ask_for_save_path_if_needed();
        if (not path) {
            return EditorState::SaveOutcome::UserCanceled; 
        } else {
            save(*path);
            return EditorState::SaveOutcome::UserSaved;
        }
    }
};

std::optional<std::filesystem::path> EditorState::ask_for_save_path_if_needed() {
    if (song_path) {
        return song_path;
    } else {
        return feis::save_file_dialog();
    }
};

void EditorState::move_backwards_in_time() {
    auto beats = current_snaped_beats();
    if (beats >= current_exact_beats()) {
        beats -= get_snap_step();
    }
    set_playback_position(beats);
};

void EditorState::move_forwards_in_time() {
    auto beats = current_snaped_beats();
    if (beats <= current_exact_beats()) {
        beats += get_snap_step();
    }
    set_playback_position(beats);
};

void EditorState::undo(NotificationsQueue& nq) {
    if (chart_state) {
        auto previous = chart_state->history.pop_previous();
        if (previous) {
            nq.push(std::make_shared<UndoNotification>(**previous));
            (*previous)->undo_action(*this);
            chart_state->density_graph.should_recompute = true;
        }
    }
};

void EditorState::redo(NotificationsQueue& nq) {
    if (chart_state) {
        auto next = chart_state->history.pop_next();
        if (next) {
            nq.push(std::make_shared<RedoNotification>(**next));
            (*next)->do_action(*this);
            chart_state->density_graph.should_recompute = true;
        }
    }
};

void EditorState::open_chart(const std::string& name) {
    auto& [name_ref, chart] = *song.charts.find(name);
    chart_state.emplace(chart, name_ref, assets);
    reload_editable_range();
    reload_applicable_timing();
    note_claps->set_notes_and_timing(&chart.notes, &applicable_timing);
    beat_ticks->set_timing(&applicable_timing);
};

void EditorState::update_visible_notes() {
    if (chart_state) {
        chart_state->update_visible_notes(
            current_time(),
            applicable_timing
        );
    }
};


void EditorState::reload_editable_range() {
    const auto old_range = this->editable_range;
    this->editable_range = choose_editable_range();
    if (old_range != this->editable_range and this->chart_state.has_value()) {
        chart_state->density_graph.should_recompute = true;
    }
};

Interval<sf::Time> EditorState::choose_editable_range() {
    Interval<sf::Time> new_range{sf::Time::Zero, sf::Time::Zero};
    if (music.has_value()) {
        // If there is music, allow editing up to the end, but no further
        // You've put notes *after* the end of the music ? fuck 'em.
        new_range += (**music).getDuration();
        return new_range;
    } else {
        // If there is no music :
        // make sure we can edit 10 seconds after the end of the current chart
        if (chart_state and not chart_state->chart.notes.empty()) {
            const auto beat_of_last_event = chart_state->chart.notes.crbegin()->second.get_end();
            new_range += time_at(beat_of_last_event) + sf::seconds(10);
        }
        // and at at least the first whole minute in any case
        new_range += sf::seconds(60);
        return new_range;
    }
}

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
    } else {
        jacket->setSmooth(true);
    }
};

/*
 * Reloads music from what's indicated in the "music path" field of the song
 * Resets the music state in case anything fails
 * Updates playbackPosition and preview_end as well
 */
void EditorState::reload_music() {
    if (not song_path.has_value() or song.metadata.audio.empty()) {
        clear_music();
        return;
    }

    const auto absolute_music_path = song_path->parent_path() / song.metadata.audio;
    try {
        music.emplace(std::make_shared<OpenMusic>(absolute_music_path));
    } catch (const std::exception& e) {
        clear_music();
    }

    reload_editable_range();
    const auto clamped_position = std::clamp(
        current_time(),
        editable_range.start,
        editable_range.end
    );
    playback_position = clamped_position;
    previous_playback_position = playback_position;
    set_speed(speed);
    if (music.has_value()) {
        audio.add_stream(music_stream, {*music, false});
    } else {
        audio.remove_stream(music_stream);
    }
    audio.setPlayingOffset(clamped_position);
};

void EditorState::clear_music() {
    audio.remove_stream(music_stream);
    music.reset();
}

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
};

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
    song_path = path;
    if (chart_state) {
        chart_state->history.mark_as_saved();
    }
};

void feis::save(
    std::optional<EditorState>& ed,
    NotificationsQueue& nq
) {
    if (ed) {
        if (ed->save_if_needed() == EditorState::SaveOutcome::UserSaved) {
            nq.push(std::make_shared<TextNotification>("Saved file"));
        }
    }
}

// SAVE if needed and the user asked to, then ASK for a file to opne, then OPEN than file
void feis::save_ask_open(
    std::optional<EditorState>& ed,
    const std::filesystem::path& assets,
    const std::filesystem::path& settings
) {
    if (ed and ed->save_if_needed_and_user_wants_to() == EditorState::SaveOutcome::UserCanceled) {
        return;
    }

    if (const auto& filepath = feis::open_file_dialog()) {
        feis::open_from_file(ed, *filepath, assets, settings);
    }
};

// SAVE if needed and the user asked to, then OPEN the file passed as argument
void feis::save_open(
    std::optional<EditorState>& ed,
    const std::filesystem::path& song_path,
    const std::filesystem::path& assets,
    const std::filesystem::path& settings
) {
    if (ed and ed->save_if_needed_and_user_wants_to() == EditorState::SaveOutcome::UserCanceled) {
        return;
    }

    feis::open_from_file(ed, song_path, assets, settings);
};


void feis::open_from_file(
    std::optional<EditorState>& ed,
    const std::filesystem::path& song_path,
    const std::filesystem::path& assets,
    const std::filesystem::path& settings
) {
    try {
        // force utf-8 song path on windows 
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
        const auto json = load_json_preserving_decimals(f);
        auto song = better::Song::load_from_memon(json);
        ed.emplace(song, assets, song_path);
        Toolbox::pushNewRecentFile(std::filesystem::canonical(song_path), settings);
    } catch (const std::exception& e) {
        tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
    }
};

void feis::save_close(std::optional<EditorState>& ed) {
    if (ed and ed->save_if_needed_and_user_wants_to() == EditorState::SaveOutcome::UserCanceled) {
        return;
    }

    ed.reset();
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
};

void feis::ChartPropertiesDialog::display(EditorState& editor_state) {
    assert(editor_state.chart_state.has_value());

    if (this->should_refresh_values) {
        should_refresh_values = false;

        difficulty_names_in_use.clear();
        this->level = editor_state.chart_state->chart.level.value_or(0);
        this->difficulty_name = editor_state.chart_state->difficulty_name;
        this->show_custom_dif_name = (
            difficulty_name != "BSC"
            and difficulty_name != "ADV"
            and difficulty_name != "EXT"
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
};
