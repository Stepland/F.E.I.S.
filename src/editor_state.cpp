#include "editor_state.hpp"

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Audio/SoundStream.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>

#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <initializer_list>
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
#include "compile_time_info.hpp"
#include "file_dialogs.hpp"
#include "history_item.hpp"
#include "imgui_extras.hpp"
#include "json_decimal_handling.hpp"
#include "long_note_dummy.hpp"
#include "notifications_queue.hpp"
#include "special_numeric_types.hpp"
#include "src/better_metadata.hpp"
#include "src/better_timing.hpp"
#include "src/custom_sfml_audio/synced_sound_streams.hpp"
#include "variant_visitor.hpp"
#include "utf8_strings.hpp"

EditorState::EditorState(const std::filesystem::path& assets_, config::Config& config_) :
    config(config_),
    note_claps(std::make_shared<NoteClaps>(nullptr, nullptr, assets_, 1.f)),
    chord_claps(std::make_shared<ChordClaps>(nullptr, nullptr, assets_, 1.f)),
    beat_ticks(std::make_shared<BeatTicks>(nullptr, assets_, 1.f)),
    playfield(assets_),
    linear_view(assets_, config_),
    applicable_timing(song.timing),
    assets(assets_)
{
    reload_music();
    reload_jacket();
};

EditorState::EditorState(
    const better::Song& song_,
    const std::filesystem::path& assets_,
    const std::filesystem::path& song_path_,
    config::Config& config_
) : 
    config(config_),
    song(song_),
    song_path(song_path_),
    note_claps(std::make_shared<NoteClaps>(nullptr, nullptr, assets_, 1.f)),
    chord_claps(std::make_shared<ChordClaps>(nullptr, nullptr, assets_, 1.f)),
    beat_ticks(std::make_shared<BeatTicks>(nullptr, assets_, 1.f)),
    playfield(assets_),
    linear_view(assets_, config_),
    applicable_timing(song.timing),
    assets(assets_)
{
    if (not song.charts.empty()) {
        auto& [name, _] = *this->song.charts.begin();
        open_chart(name);
    }
    history.mark_as_saved();
    reload_music();
    reload_jacket();
    reload_preview_audio();
};

int EditorState::get_volume() const {
    return volume;
}

void EditorState::set_volume(int volume_) {
    if (music.has_value()) {
        (**music).set_volume(volume_);
        volume = (**music).get_volume();
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

void EditorState::play_music_preview() {
    if (song.metadata.use_preview_file) {
        if (preview_audio) {
            if (preview_audio->getStatus() == sf::Music::Playing) {
                preview_audio->stop();
            } else {
                preview_audio->stop();
                preview_audio->play();
            }
        }
    } else {
        set_playback_position(sf::seconds(std::stod(song.metadata.preview_loop.start.format("f"))));
        play();
        is_playing_preview_music_from_sss = true;
    }
}

void EditorState::stop_music_preview() {
    if (song.metadata.use_preview_file) {
        if (preview_audio) {
            preview_audio->stop();
        }
    } else {
        stop();
        is_playing_preview_music_from_sss = false;
    }
}

bool EditorState::music_preview_is_playing() const {
    if (song.metadata.use_preview_file) {
        if (preview_audio) {
            return preview_audio->getStatus() == sf::Music::Playing;
        } else {
            return false;
        }
    } else {
        return is_playing_preview_music_from_sss;
    }
}

sf::Time EditorState::music_preview_position() const {
    if (not music_preview_is_playing()) {
        return sf::Time::Zero;
    }

    if (song.metadata.use_preview_file) {
        if (preview_audio) {
            return preview_audio->getPlayingOffset();
        }
    } else if (is_playing_preview_music_from_sss) {
        return audio.getPlayingOffset() - sf::seconds(std::stod(song.metadata.preview_loop.start.format("f")));
    }

    return sf::Time::Zero;
}

sf::Time EditorState::music_preview_duration() const {
    if (song.metadata.use_preview_file) {
        if (preview_audio) {
            return preview_audio->getDuration();
        }
    } else {
        return sf::seconds(std::stod(song.metadata.preview_loop.duration.format("f")));
    }
    return sf::Time::Zero;
}

void EditorState::update_music_preview_status() {
    if (is_playing_preview_music_from_sss) {
        if (music_preview_position() > music_preview_duration()) {
            stop_music_preview();
        }
    }
}


const Interval<sf::Time>& EditorState::get_editable_range() {
    reload_editable_range();
    return editable_range;
};

bool EditorState::has_any_audio() const {
    return not audio.empty();
}

void EditorState::toggle_playback() {
    if (get_status() != sf::SoundSource::Playing) {
        play();
    } else {
        pause();
    }
}

void EditorState::toggle_note_claps() {
    if (
        audio.contains_stream(note_clap_stream)
        or audio.contains_stream(chord_clap_stream)
    ) {
        audio.update_streams({}, {note_clap_stream, chord_clap_stream});
    } else {
        note_claps = note_claps->with_params(
            get_pitch(),
            not distinct_chord_clap,
            clap_on_long_note_ends
        );
        std::map<std::string, NewStream> streams = {{note_clap_stream, {note_claps, true}}};
        if (distinct_chord_clap) {
            chord_claps = chord_claps->with_pitch(get_pitch());
            streams[chord_clap_stream] = {chord_claps, true};
        }
        audio.update_streams(streams);
    }
}

void EditorState::toggle_clap_on_long_note_ends() {
    clap_on_long_note_ends = not clap_on_long_note_ends;
    note_claps = note_claps->with_params(
        get_pitch(),
        not distinct_chord_clap,
        clap_on_long_note_ends
    );
    audio.update_streams({{note_clap_stream, {note_claps, true}}});
}

void EditorState::toggle_distinct_chord_claps() {
    distinct_chord_clap = not distinct_chord_clap;
    note_claps = note_claps->with_params(
        get_pitch(),
        not distinct_chord_clap,
        clap_on_long_note_ends
    );
    if (distinct_chord_clap) {
        chord_claps = chord_claps->with_pitch(get_pitch());
        audio.update_streams(
            {
                {note_clap_stream, {note_claps, true}},
                {chord_clap_stream, {chord_claps, true}}
            }
        );
    } else {
        audio.update_streams(
            {{note_clap_stream, {note_claps, true}}},
            {chord_clap_stream}
        );
    }
}

void EditorState::toggle_beat_ticks() {
    if (audio.contains_stream(beat_tick_stream)) {
        audio.remove_stream(beat_tick_stream);
    } else {
        beat_ticks = beat_ticks->with_pitch(get_pitch());
        audio.add_stream(beat_tick_stream, {beat_ticks, true});
    }
}

void EditorState::play() {
    status = sf::SoundSource::Playing;
    audio.play();
}

void EditorState::pause() {
    status = sf::SoundSource::Paused;
    audio.pause();
}

void EditorState::stop() {
    status = sf::SoundSource::Stopped;
    audio.stop();
}

sf::SoundSource::Status EditorState::get_status() {
    if (has_any_audio()) {
        return audio.getStatus();
    } else {
        return status;
    }
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
    if (audio.contains_stream(chord_clap_stream)) {
        chord_claps = chord_claps->with_pitch(pitch);
        update[chord_clap_stream] = {chord_claps, true};
    }
    audio.update_streams(update, {}, pitch);
}

float EditorState::get_pitch() const {
    return speed / 10.f;
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
    return applicable_timing->beats_at(time);
};

sf::Time EditorState::time_at(Fraction beat) const {
    return applicable_timing->time_at(beat);
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

    if (ImGui::Begin("Playfield", &show_playfield, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
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
                    make_long_note_dummy_for_playfield(
                        current_exact_beats(),
                        *chart_state->long_note_being_created,
                        get_snap_step()
                    ),
                    current_time(),
                    *applicable_timing
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
                        *applicable_timing,
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
                            *applicable_timing
                        );
                        reload_sounds_that_depend_on_notes();
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
            // Check for real (+ potential if requested) collisions
            // then display them
            std::array<bool, 16> collisions = {};
            for (const auto& [_, note] : chart_state->visible_notes) {
                if (chart_state->chart.notes->is_colliding(note, *applicable_timing, config.editor.collision_zone)) {
                    collisions[note.get_position().index()] = true;
                }
            }
            if (config.editor.show_free_buttons) {
                for (unsigned int i = 0; i < 16; i++) {
                    unsigned int x = i % 4;
                    unsigned int y = i / 4;
                    if (chart_state->chart.notes->would_collide(
                        better::TapNote{current_exact_beats(), {x, y}},
                        *applicable_timing,
                        config.editor.collision_zone
                    )) {
                        collisions[i] = true;
                    }
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
                if (chart_state->selected_stuff.notes.contains(note)) {
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
void EditorState::display_file_properties() {
    if (ImGui::Begin(
        "File Properties",
        &show_file_properties,
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

        auto edited_title = song.metadata.title;
        ImGui::InputText("Title", &edited_title);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            history.push(std::make_shared<ChangeTitle>(song.metadata.title, edited_title));
            song.metadata.title = edited_title;
        }
        auto edited_artist = song.metadata.artist;
        ImGui::InputText("Artist", &edited_artist);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            history.push(std::make_shared<ChangeArtist>(song.metadata.artist, edited_artist));
            song.metadata.artist = edited_artist;
        }

        auto edited_audio = song.metadata.audio;
        feis::InputTextColored(
            "Audio",
            &edited_audio,
            music.has_value(),
            "Invalid Audio Path"
        );
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            history.push(std::make_shared<ChangeAudio>(song.metadata.audio, edited_audio));
            song.metadata.audio = edited_audio;
            reload_music();
        }
        auto edited_jacket = song.metadata.jacket;
        feis::InputTextColored(
            "Jacket",
            &edited_jacket,
            jacket.has_value(),
            "Invalid Jacket Path"
        );
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            history.push(std::make_shared<ChangeJacket>(song.metadata.jacket, edited_jacket));
            song.metadata.jacket = edited_jacket;
            reload_jacket();
        }

        ImGui::Separator();
        
        ImGui::Text("Preview");
        if (ImGui::Checkbox("Use separate preview file", &song.metadata.use_preview_file)) {
            stop_music_preview();
            if (song.metadata.use_preview_file) {
                history.push(std::make_shared<ChangePreview>(song.metadata.preview_loop, song.metadata.preview_file));
            } else {
                history.push(std::make_shared<ChangePreview>(song.metadata.preview_file, song.metadata.preview_loop));
            }
        };
        if (song.metadata.use_preview_file) {
            auto edited_preview_file = song.metadata.preview_file;
            feis::InputTextColored(
                "File",
                &edited_preview_file,
                preview_audio.has_value(),
                "Invalid Path"
            );
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                history.push(std::make_shared<ChangePreview>(song.metadata.preview_file, edited_preview_file));
                song.metadata.preview_file = edited_preview_file;
                reload_preview_audio();
            }
        } else {
            auto edited_loop_start = song.metadata.preview_loop.start;
            feis::InputDecimal(
                "Start", 
                &edited_loop_start
            );
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                edited_loop_start = std::max(
                    Decimal{0},
                    edited_loop_start
                );
                if (music.has_value()) {
                    edited_loop_start = std::min(
                        Decimal{(**music).getDuration().asMicroseconds()} / 1000000,
                        edited_loop_start
                    );
                }
                history.push(std::make_shared<ChangePreview>(
                    song.metadata.preview_loop,
                    better::PreviewLoop{edited_loop_start, song.metadata.preview_loop.duration}
                ));
                song.metadata.preview_loop.start = edited_loop_start;
            }
            auto edited_loop_duration = song.metadata.preview_loop.duration;
            feis::InputDecimal(
                "Duration",
                &edited_loop_duration
            );
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                edited_loop_duration = std::max(
                    Decimal{0},
                    edited_loop_duration
                );
                if (music.has_value()) {
                    edited_loop_duration = std::min(
                        (
                            Decimal{
                                (**music)
                                .getDuration()
                                .asMicroseconds()
                            } / 1000000 
                            - song.metadata.preview_loop.start
                        ),
                        edited_loop_duration
                    );
                }
                history.push(std::make_shared<ChangePreview>(
                    song.metadata.preview_loop,
                    better::PreviewLoop{song.metadata.preview_loop.start, edited_loop_duration}
                ));
                song.metadata.preview_loop.duration = edited_loop_duration;
            }
            const bool should_display_quick_define = chart_state and chart_state->time_selection;
            if (not should_display_quick_define) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Define from time selection")) {
                stop_music_preview();
                const auto before = [&]() -> PreviewState {
                    if (song.metadata.use_preview_file) {
                        return song.metadata.preview_file;
                    } else {
                        return song.metadata.preview_loop;
                    }
                }();
                const auto start = time_at(chart_state->time_selection->start);
                const auto end = time_at(chart_state->time_selection->end);
                const auto duration = end - start;
                song.metadata.use_preview_file = false;
                song.metadata.preview_loop.start = Decimal{fmt::format("{:.03}", start.asSeconds())};
                song.metadata.preview_loop.duration = Decimal{fmt::format("{:.03}", duration.asSeconds())};
                history.push(std::make_shared<ChangePreview>(before, song.metadata.preview_loop));
            }
            if (not should_display_quick_define) {
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "You must define a time selection in the linear view first !\n"
                        "Open up 'View' > 'Linear View' then use the Tab key to set the start, then the end"
                    );
                    ImGui::EndTooltip();
                }
            }
        }
        if (music_preview_is_playing()) {
            if (feis::StopButton("##Stop Mucic Preview")) {
                stop_music_preview();
            }
        } else {
            if (ImGui::ArrowButton("##Play Music Preview", ImGuiDir_Right)) {
                play_music_preview();
            }
        }
        ImGui::SameLine();
        const auto position = music_preview_position();
        const auto duration = music_preview_duration();
        float progress = [&](){
            if (duration == sf::Time::Zero) {
                return 0.f;
            } else {
                return position / duration;
            }
        }();
        ImGui::PushItemWidth(ImGui::CalcItemWidth() - ImGui::GetCursorPosX() + ImGui::GetStyle().WindowPadding.x);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::SliderFloat(
            "##Progress",
            &progress,
            0.f,
            1.f,
            fmt::format(
                "{:02}:{:02} / {:02}:{:02}",
                static_cast<unsigned int>(position / sf::seconds(60)),
                static_cast<unsigned int>(position.asSeconds()) % 60,
                static_cast<unsigned int>(duration / sf::seconds(60)),
                static_cast<unsigned int>(duration.asSeconds()) % 60
            ).c_str()
        );
        ImGui::PopItemFlag();
        ImGui::PopItemWidth();
    }
    ImGui::End();
};

/*
Display any information that would be useful for the user to troubleshoot the
status of the editor. Will appear in the "Editor Status" window
*/
void EditorState::display_status() {
    ImGui::Begin("Status", &show_status, ImGuiWindowFlags_AlwaysAutoResize);
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

        audio.display_debug();
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
        &show_playback_status,
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
            *applicable_timing,
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
        &show_timeline,
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
    if (ImGui::Begin("Chart List", &show_chart_list, ImGuiWindowFlags_AlwaysAutoResize)) {
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
                ImGui::Text("%d", static_cast<int>(chart.notes->size()));
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
    if (
        ImGui::Begin(
            "Linear View", 
            &show_linear_view, 
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        )
    ) {
        if (chart_state) {
            auto header_height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            ImGui::SetCursorPos({0, header_height});
            linear_view.draw(
                ImGui::GetWindowDrawList(),
                *chart_state,
                *applicable_timing,
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

void EditorState::display_sound_settings() {
    if (ImGui::Begin("Sound Settings", &show_sound_settings)) {
        if (ImGui::TreeNodeEx("Music", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginDisabled(not music.has_value());
            {
                auto volume = get_volume();
                if (ImGui::SliderInt("Volume##Music", &volume, 0, 10)) {
                    set_volume(volume);
                }
            }
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Beat Tick", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool beat_tick = beat_ticks_are_on();
            if (ImGui::Checkbox("On/Off##Beat Tick", &beat_tick)) {
                toggle_beat_ticks();
            }
            ImGui::BeginDisabled(not beat_tick);
            {
                auto volume = beat_ticks->get_volume();
                if (ImGui::SliderInt("Volume##Beat Tick", &volume, 0, 10)) {
                    beat_ticks->set_volume(volume);
                }
            }
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Note Clap", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool note_clap = note_claps_are_on();
            if (ImGui::Checkbox("On/Off##Note Clap", &note_clap)) {
                toggle_note_claps();
            }
            ImGui::BeginDisabled(not note_clap);
            {
                auto volume = note_claps->get_volume();
                if (ImGui::SliderInt("Volume##Note Clap", &volume, 0, 10)) {
                    note_claps->set_volume(volume);
                    chord_claps->set_volume(volume);
                }
                if (ImGui::TreeNode("Advanced##Note Clap")) {
                    bool long_end = get_clap_on_long_note_ends();
                    if (ImGui::Checkbox("Clap on long note ends", &long_end)) {
                        toggle_clap_on_long_note_ends();
                    }
                    bool chord_clap = get_distinct_chord_claps();
                    if (ImGui::Checkbox("Distinct chord clap", &chord_clap)) {
                        toggle_distinct_chord_claps();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void EditorState::display_editor_settings() {
    if (
        ImGui::Begin(
            "Editor Settings",
            &show_editor_settings,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize
        )
    ) {
        static const std::uint64_t step = 1;
        if (ImGui::InputScalar("Snap##Editor Settings", ImGuiDataType_U64, &snap, &step, nullptr, "%d")) {
            snap = std::clamp(snap, UINT64_C(1), UINT64_C(1000));
        };
        ImGui::SameLine();
        feis::HelpMarker(
            "Change the underlying snap value, this allows setting snap "
            "values that aren't a divisor of 240. "
            "This changes the underlying value that's multiplied "
            "by 4 before being shown in the status bar"
        );
        int collision_zone_ms = config.editor.collision_zone.asMilliseconds();
        if (ImGui::SliderInt("Collision Zone##Editor Settings", &collision_zone_ms, 100, 2000, "%d ms")) {
            collision_zone_ms = std::clamp(collision_zone_ms, 100, 2000);
            config.editor.collision_zone = sf::milliseconds(collision_zone_ms);
            if (chart_state) {
                chart_state->density_graph.should_recompute = true;
            }
        }
        ImGui::SameLine();
        feis::HelpMarker(
            "Suggested minimal duration between two notes on the same "
            "button.\n"
            "If two notes are closer than this they \"collide\" and are "
            "highlighted in red everywhere"
        );
        const std::array<std::pair<const char*, sf::Time>, 3> presets{{
            {"F.E.I.S default", sf::seconds(1)},
            {"Safe", sf::milliseconds(1066)},
            {"jubeat plus", sf::milliseconds(1030)}
        }};
        if (ImGui::BeginCombo("Collision Zone Presets", "Choose ...")) {
            for (const auto& [name, value] : presets) {
                if (ImGui::Selectable(name, false)) {
                    config.editor.collision_zone = value;
                    if (chart_state) {
                        chart_state->density_graph.should_recompute = true;
                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox("Show Free Buttons", &config.editor.show_free_buttons);
    }
    ImGui::End();
}

void EditorState::display_history() {
    history.display(show_history);
}

void EditorState::display_timing_menu() {
    if (ImGui::Begin("Adjust Timing", &show_timing_menu)) {
        auto bpm = std::visit(
            [&](const auto& pos){return applicable_timing->bpm_at(pos);},
            playback_position
        );
        if (feis::InputDecimal("BPM", &bpm, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (bpm > 0) {
                const auto before = *applicable_timing;
                applicable_timing->insert(better::BPMAtBeat{bpm, current_snaped_beats()});
                if (*applicable_timing != before) {
                    reload_sounds_that_depend_on_timing();
                    history.push(std::make_shared<ChangeTiming>(before, *applicable_timing, timing_origin()));
                }
                if (chart_state) {
                    chart_state->density_graph.should_recompute = true;
                }
            }
        }
        auto offset = applicable_timing->get_offset();
        if (feis::InputDecimal("beat zero offset", &offset, ImGuiInputTextFlags_EnterReturnsTrue)) {
            applicable_timing->set_offset(offset);
            reload_sounds_that_depend_on_timing();
            set_playback_position(current_exact_beats());
            if (chart_state) {
                chart_state->density_graph.should_recompute = true;
            }
        }
    }
    ImGui::End();
}

bool EditorState::needs_to_save() const {
    return not history.current_state_is_saved();
};

EditorState::UserWantsToSave EditorState::ask_if_user_wants_to_save() const {
    int response_code = tinyfd_messageBox(
        "Warning",
        "The currently open chart has unsaved changes, do you want to save ?",
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
            return save_asking_for_path();
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
        return save_asking_for_path();
    }
};

EditorState::SaveOutcome EditorState::save_asking_for_path() {
    const auto path = ask_for_save_path_if_needed();
    if (not path) {
        return EditorState::SaveOutcome::UserCanceled; 
    } else {
        save(*path);
        return EditorState::SaveOutcome::UserSaved;
    }
};

std::optional<std::filesystem::path> EditorState::ask_for_save_path_if_needed() {
    if (song_path) {
        return song_path;
    } else {
        return feis::save_file_dialog();
    }
};

void EditorState::insert_long_note_just_created() {
    if (not chart_state) {
        return;
    }
    chart_state->insert_long_note_just_created(snap);
    reload_sounds_that_depend_on_notes();
}

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
    auto previous = history.pop_previous();
    if (previous) {
        nq.push(std::make_shared<UndoNotification>(**previous));
        (*previous)->undo_action(*this);
    }
};

void EditorState::redo(NotificationsQueue& nq) {
    auto next = history.pop_next();
    if (next) {
        nq.push(std::make_shared<RedoNotification>(**next));
        (*next)->do_action(*this);
    }
};

void EditorState::cut(NotificationsQueue& nq) {
    if (chart_state) {
        chart_state->cut(nq, *applicable_timing, timing_origin());
        reload_all_sounds();
    }
}

void EditorState::copy(NotificationsQueue& nq) {
    if (chart_state) {
        chart_state->copy(nq);
    }
}

void EditorState::paste(NotificationsQueue& nq) {
    if (chart_state) {
        chart_state->paste(
            current_snaped_beats(),
            nq,
            *applicable_timing,
            timing_origin()
        );
    }
}

void EditorState::delete_(NotificationsQueue& nq) {
    if (chart_state) {
        chart_state->delete_(nq, *applicable_timing, timing_origin());
    }
}

// Discard, in that order :
// - time selection
// - notes & bpm event selection
void EditorState::discard_selection() {
    if (chart_state) {
        if (chart_state->time_selection) {
            chart_state->time_selection.reset();
        } else if (not chart_state->selected_stuff.empty()) {
            chart_state->selected_stuff.clear();
        }
    }
}

void EditorState::insert_chart(const std::string& name, const better::Chart& chart) {
    if (song.charts.try_emplace(name, chart).second) {
        open_chart(name);
    }
}

void EditorState::insert_chart_and_push_history(const std::string& name, const better::Chart& chart) {
    insert_chart(name, chart);
    history.push(std::make_shared<AddChart>(name, chart));
}

void EditorState::erase_chart(const std::string& name) {
    const auto& it = song.charts.find(name);
    if (it != song.charts.end()) {
        if (chart_state and name == chart_state->difficulty_name) {
            close_chart();
        }
        song.charts.erase(name);
    }
}


void EditorState::erase_chart_and_push_history(const std::string& name) {
    const auto& it = song.charts.find(name);
    if (it != song.charts.end()) {
        if (chart_state and name == chart_state->difficulty_name) {
            close_chart();
        }
        history.push(std::make_shared<RemoveChart>(name, it->second));
        song.charts.erase(name);
    }
}

void EditorState::open_chart(const std::string& name) {
    auto& [name_ref, chart] = *song.charts.find(name);
    chart_state.emplace(chart, name_ref, history, assets, config);
    reload_editable_range();
    reload_applicable_timing();
    reload_all_sounds();
};

void EditorState::close_chart() {
    chart_state.reset();
};

void EditorState::update_visible_notes() {
    if (chart_state) {
        chart_state->update_visible_notes(
            current_time(),
            *applicable_timing
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
        if (chart_state and not chart_state->chart.notes->empty()) {
            const auto beat_of_last_event = chart_state->chart.notes->crbegin()->second.get_end();
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
        or not jacket->load_from_path(jacket_path)
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
    const auto status_before = get_status();
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
    set_speed(speed);
    if (music.has_value()) {
        audio.add_stream(music_stream, {*music, false});
    } else {
        audio.remove_stream(music_stream);
    }
    pause();
    set_playback_position(current_time());
    switch (status_before) {
        case sf::SoundSource::Playing:
            play();
            break;
        case sf::SoundSource::Paused:
            pause();
            break;
        case sf::SoundSource::Stopped:
            stop();
            break;
    }
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
    if (not preview_audio->open_from_path(path)) {
        preview_audio.reset();
    }
};

void EditorState::reload_sounds_that_depend_on_notes() {
    std::map<std::string, NewStream> update;
    const auto pitch = get_pitch();
    std::shared_ptr<better::Notes> notes = [&](){
        if (chart_state) {
            return chart_state->chart.notes;
        } else {
            return std::shared_ptr<better::Notes>{};
        }
    }();
    note_claps = (
        note_claps
        ->with_pitch(pitch)
        ->with_notes_and_timing(notes, applicable_timing)
    );
    if (audio.contains_stream(note_clap_stream)) {
        update[note_clap_stream] = {note_claps, true};
    }
    chord_claps = (
        chord_claps
        ->with_pitch(pitch)
        ->with_notes_and_timing(notes, applicable_timing)
    );
    if (audio.contains_stream(chord_clap_stream)) {
        update[chord_clap_stream] = {chord_claps, true};
    }
    audio.update_streams(update, {}, pitch);
}

void EditorState::reload_sounds_that_depend_on_timing() {
    std::map<std::string, NewStream> update;
    const auto pitch = get_pitch();
    std::shared_ptr<better::Notes> notes = [&](){
        if (chart_state) {
            return chart_state->chart.notes;
        } else {
            return std::shared_ptr<better::Notes>{};
        }
    }();
    note_claps = (
        note_claps
        ->with_pitch(pitch)
        ->with_notes_and_timing(notes, applicable_timing)
    );
    if (audio.contains_stream(note_clap_stream)) {
        update[note_clap_stream] = {note_claps, true};
    }

    beat_ticks = (
        beat_ticks
        ->with_pitch(get_pitch())
        ->with_timing(applicable_timing)
    );
    if (audio.contains_stream(beat_tick_stream)) {
        update[beat_tick_stream] = {beat_ticks, true};
    }
    chord_claps = (
        chord_claps
        ->with_pitch(pitch)
        ->with_notes_and_timing(notes, applicable_timing)
    );
    if (audio.contains_stream(chord_clap_stream)) {
        update[chord_clap_stream] = {chord_claps, true};
    }
    audio.update_streams(update, {}, pitch);
}

void EditorState::reload_all_sounds() {
    reload_sounds_that_depend_on_timing();
}


void EditorState::reload_applicable_timing() {
    if (chart_state and chart_state->chart.timing) {
        applicable_timing = *chart_state->chart.timing;
    } else {
        applicable_timing = song.timing;
    }
};

TimingOrigin EditorState::timing_origin() {
    if (chart_state and chart_state->chart.timing) {
        return chart_state->difficulty_name;
    } else {
        return GlobalTimingObject{};
    }
}

void EditorState::save(const std::filesystem::path& path) {
    const auto memon = song.dump_to_memon_1_0_0();
    nowide::ofstream file{to_utf8_encoded_string(path.string())};
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
    history.mark_as_saved();
};

void feis::force_save(
    std::optional<EditorState>& ed,
    NotificationsQueue& nq
) {
    if (ed) {
        if (ed->save_asking_for_path() == EditorState::SaveOutcome::UserSaved) {
            nq.push(std::make_shared<TextNotification>("Saved file"));
        }
    }
}

// SAVE if needed and the user asked to, then ASK for a file to opne, then OPEN than file
void feis::save_ask_open(
    std::optional<EditorState>& ed,
    const std::filesystem::path& assets,
    const std::filesystem::path& settings,
    config::Config& config
) {
    if (ed and ed->save_if_needed_and_user_wants_to() == EditorState::SaveOutcome::UserCanceled) {
        return;
    }

    if (const auto& filepath = feis::open_file_dialog()) {
        feis::open_from_file(ed, *filepath, assets, settings, config);
    }
};

// SAVE if needed and the user asked to, then OPEN the file passed as argument
void feis::save_open(
    std::optional<EditorState>& ed,
    const std::filesystem::path& song_path,
    const std::filesystem::path& assets,
    const std::filesystem::path& settings,
    config::Config& config
) {
    if (ed and ed->save_if_needed_and_user_wants_to() == EditorState::SaveOutcome::UserCanceled) {
        return;
    }

    feis::open_from_file(ed, song_path, assets, settings, config);
};


void feis::open_from_file(
    std::optional<EditorState>& ed,
    const std::filesystem::path& song_path,
    const std::filesystem::path& assets,
    const std::filesystem::path& settings,
    config::Config& config
) {
    try {
        nowide::ifstream f{to_utf8_encoded_string(song_path)};
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
        ed.emplace(song, assets, song_path, config);
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
            &editorState.show_new_chart_dialog,
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
            &editor_state.show_chart_properties,
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
                const auto old_dif_name = editor_state.chart_state->difficulty_name;
                const auto old_level = editor_state.chart_state->chart.level;
                try {
                    auto modified_chart = editor_state.song.charts.extract(editor_state.chart_state->difficulty_name);
                    modified_chart.key() = this->difficulty_name;
                    modified_chart.mapped().level = this->level;
                    const auto [_1, inserted, _2] = editor_state.song.charts.insert(std::move(modified_chart));
                    if (not inserted) {
                        throw std::runtime_error("Could not insert modified chart in song");
                    } else {
                        editor_state.open_chart(this->difficulty_name);
                        if (old_dif_name != this->difficulty_name) {
                            editor_state.history.push(
                                std::make_shared<RenameChart>(
                                    old_dif_name,
                                    this->difficulty_name
                                )
                            );
                        }
                        if (old_level != this->level) {
                            editor_state.history.push(
                                std::make_shared<RerateChart>(
                                    this->difficulty_name,
                                    old_level,
                                    this->level
                                )
                            );
                        }
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

void feis::display_shortcuts_help(bool& show) {
    const auto table_shortcut = [](const char* action, const char* keys){
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(action);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(keys);
    };
    const auto table_header = [&](){
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 175.f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
    };
    if (ImGui::Begin("Shortcuts", &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Navigation");
        if (
            ImGui::BeginTable(
                "Navigation",
                2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            )
        ) {
            table_header();
            table_shortcut("Play / Pause", "Space");
            table_shortcut("Move Backwards In Time", "Down");
            table_shortcut("Move Forwards In Time", "Up");
            table_shortcut("Decrease Snap", "Left");
            table_shortcut("Increase Snap", "Right");
            ImGui::EndTable();
        }
        ImGui::Text("Playfield");
        if (
            ImGui::BeginTable(
                "Playfield",
                2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            )
        ) {
            table_header();
            table_shortcut("Show Free Buttons", "F");
            ImGui::EndTable();
        }

        ImGui::Text("Linear View");
        if (
            ImGui::BeginTable(
                "Linear View",
                2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            )
        ) {
            table_header();
            table_shortcut("Zoom In", "Numpad +");
            table_shortcut("Zoom Out", "Numpad -");
            table_shortcut("Set Time Selection Bounds", "Tab");
            table_shortcut("Discard Selection", "Escape");
            ImGui::EndTable();
        }

        ImGui::Text("Editing");
        if (
            ImGui::BeginTable(
                "Editing",
                2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            )
        ) {
            table_header();
            table_shortcut("Cut", "Ctrl+X");
            table_shortcut("Copy", "Ctrl+C");
            table_shortcut("Paste", "Ctrl+V");
            table_shortcut("Delete", "Delete");
            ImGui::EndTable();
        }

        ImGui::Text("Sound");
        if (
            ImGui::BeginTable(
                "Sound",
                2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            )
        ) {
            table_header();
            table_shortcut("Increase Music Volume", "Shift+Up");
            table_shortcut("Decrease Music Volume", "Shift+Down");
            table_shortcut("Slow Down Playback", "Shift+Left");
            table_shortcut("Speed Up Playback", "Shift+Right");
            table_shortcut("Toggle Beat Tick", "F3");
            table_shortcut("Toggle Note Tick", "F4");
            table_shortcut("Toggle Chord Tick", "Shift+F4");
            ImGui::EndTable();
        }
    }
    ImGui::End();
};

const std::vector<std::vector<sf::Vector2f>> lines = {
    {{1, 5}, {1, 1}, {4, 1}}, // F
    {{1, 3}, {3, 3}},
    {{4.8f, 4.8f}, {5.2f, 4.8f}, {5.2f, 5.2f}, {4.8f, 5.2f}, {4.8f, 4.8f}}, // .
    {{9, 5}, {6, 5}, {6, 1}, {9, 1}}, // E
    {{6, 3}, {8, 3}},
    {{9.8f, 4.8f}, {10.2f, 4.8f}, {10.2f, 5.2f}, {9.8f, 5.2f}, {9.8f, 4.8f}}, // .
    {{11, 1}, {14, 1}}, // I
    {{12.5f, 1}, {12.5f, 5}},
    {{11, 5}, {14, 5}},
    {{14.8f, 4.8f}, {15.2f, 4.8f}, {15.2f, 5.2f}, {14.8f, 5.2f}, {14.8f, 4.8f}}, // .
    {{16, 4.5f}, {16, 5}, {19, 5}, {19, 3}, {16, 3}, {16, 1}, {19, 1}, {19, 1.5f}} // S
};

const std::vector<float> random_offsets = {
    +0.34f, -0.11f, +0.28f, -0.13f, -0.09f,
    +0.03f, +0.22f, +0.01f, +0.13f, +0.15f,
    +0.23f, -0.16f, -0.28f,	+0.08f, -0.23f,
    +0.05f, +0.22f, -0.03f, +0.22f, +0.46f
};

void feis::display_about_menu(bool &show) {
    static float scale = 10.0f;
    static float noise_scale = 0.65f;
    static sf::Vector2f offset = {19, 60};
    static sf::Vector2f dummy_size = {225, 125};
    static std::size_t index = 0;
    /*
    if (ImGui::Begin("About Debug")) {
        ImGui::SliderFloat("Scale", &scale, 0, 10);
        ImGui::SliderFloat("Noise scale", &noise_scale, 0, 1, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat2("Offset", &offset.x, 0, 200);
        ImGui::SliderFloat2("Dummy", &dummy_size.x, 0, 300);
    }
    ImGui::End();
    */
    if (ImGui::Begin("About", &show, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Dummy(dummy_size);
        auto drawlist = ImGui::GetWindowDrawList();
        auto origin = sf::Vector2f{ImGui::GetWindowPos()};
        for (const auto& points : lines) {
            for (auto point = points.begin(); point != points.end(); ++point) {
                auto next_point = std::next(point);
                if (next_point != points.end()) {
                    const auto random_offset = sf::Vector2f{
                        random_offsets.at(index % random_offsets.size()),
                        random_offsets.at((index + 1) % random_offsets.size())
                    };
                    const auto next_random_offset = sf::Vector2f{
                        random_offsets.at((index + 2) % random_offsets.size()),
                        random_offsets.at((index + 3) % random_offsets.size()),
                    };
                    drawlist->AddLine(
                        origin +((*point + (random_offset * noise_scale)) * scale) + offset,
                        origin +((*next_point + (next_random_offset * noise_scale)) * scale) + offset,
                        ImColor(sf::Color::White)
                    );
                    index += 2;
                    index %= random_offsets.size();
                }
            }
        }
        feis::CenteredText(fmt::format("{} {}", FEIS_NAME, FEIS_VERSION));
        feis::CenteredText("Made with <3 by Stepland");
    }
    ImGui::End();
}
