#include <SFML/Audio/SoundFileFactory.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fmt/core.h>
#include <future>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <tinyfiledialogs.h>
#include <tuple>
#include <variant>
#include <whereami++.hpp>

#include "chart_state.hpp"
#include "config.hpp"
#include "editor_state.hpp"
#include "file_dialogs.hpp"
#include "history_item.hpp"
#include "imgui_extras.hpp"
#include "imgui_internal.h"
#include "marker.hpp"
#include "markers.hpp"
#include "mp3_reader.hpp"
#include "notifications_queue.hpp"
#include "src/custom_sfml_audio/synced_sound_streams.hpp"
#include "utf8_sfml.hpp"
#include "utf8_strings.hpp"
#include "widgets/blank_screen.hpp"

int main() {
    // extend SFML to be able to read mp3's
    sf::SoundFileFactory::registerReader<sf::priv::SoundFileReaderMp3>();

    const auto executable_folder = utf8_encoded_string_to_path(whereami::executable_dir());
    const auto assets_folder = executable_folder / "assets";
    const auto settings_folder = executable_folder / "settings";
    const auto markers_folder = assets_folder / "textures" / "markers";

    config::Config config{settings_folder};
    const auto video_mode = sf::VideoMode{config.windows.main_window_size.x, config.windows.main_window_size.y};
    sf::RenderWindow window{video_mode, "F.E.I.S"};
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);

    ImGui::SFML::Init(window, false);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 5.f;

    auto font_path = assets_folder / "fonts" / "NotoSans-Medium.ttf";
    if (not std::filesystem::exists(font_path)) {
        tinyfd_messageBox(
            "Error",
            ("Could not open " + path_to_utf8_encoded_string(font_path)).c_str(),
            "ok",
            "error",
            1);
        return -1;
    }
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->Clear();
    IO.Fonts->AddFontFromFileTTF(
        path_to_utf8_encoded_string(assets_folder / "fonts" / "NotoSans-Medium.ttf")
            .c_str(),
        16.f);
    ImGui::SFML::UpdateFontTexture();

    IO.ConfigWindowsMoveFromTitleBarOnly = true;

    Markers markers{markers_folder, config};
    if (not config.marker.ending_state) {
        config.marker.ending_state = Judgement::Perfect;
    }
    Judgement& markerEndingState = *config.marker.ending_state;

    BlankScreen bg{assets_folder};
    std::optional<EditorState> editor_state;
    NotificationsQueue notificationsQueue;
    feis::NewChartDialog newChartDialog;
    feis::ChartPropertiesDialog chartPropertiesDialog;
    bool show_shortcuts_help = false;
    bool show_about_menu = false;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        if (editor_state) {
            editor_state->frame_hook();
        }
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            switch (event.type) {
                case sf::Event::Closed:
                    if (editor_state) {
                        if (editor_state->save_if_needed_and_user_wants_to()
                            != EditorState::SaveOutcome::UserCanceled) {
                            window.close();
                        }
                    } else {
                        window.close();
                    }
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View(
                        sf::FloatRect(0, 0, event.size.width, event.size.height)));
                    config.windows.main_window_size = window.getSize();
                    break;
                case sf::Event::MouseButtonPressed:
                    switch (event.mouseButton.button) {
                        case sf::Mouse::Button::Right:
                            if (editor_state and editor_state->chart_state) {
                                editor_state->chart_state->creating_long_note = true;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::MouseButtonReleased:
                    switch (event.mouseButton.button) {
                        case sf::Mouse::Button::Right:
                            if (editor_state) {
                                editor_state->insert_long_note_just_created();
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::MouseWheelScrolled:
                    switch (event.mouseWheelScroll.wheel) {
                        case sf::Mouse::Wheel::VerticalWheel: {
                            if (ImGui::GetIO().WantCaptureMouse) {
                                const auto window = ImGui::GetCurrentContext()->HoveredWindow;
                                if (window and not(window->Flags & ImGuiWindowFlags_NoScrollWithMouse)) {
                                    break;
                                }
                            }
                            if (editor_state) {
                                auto delta = static_cast<int>(
                                    std::floor(event.mouseWheelScroll.delta));
                                if (delta >= 0) {
                                    for (int i = 0; i < delta; ++i) {
                                        editor_state->move_backwards_in_time();
                                    }
                                } else {
                                    for (int i = 0; i < -delta; ++i) {
                                        editor_state->move_forwards_in_time();
                                    }
                                }
                            }
                        } break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::KeyPressed:
                    if (ImGui::GetIO().WantCaptureKeyboard) {
                        break;
                    }
                    switch (event.key.code) {
                        /*
                         * Selection related stuff
                         */

                        // Discard, in that order :
                        // - time selection
                        // - selected notes
                        case sf::Keyboard::Escape:
                            if (editor_state) {
                                editor_state->discard_selection();
                            }
                            break;

                        // Modify time selection
                        case sf::Keyboard::Tab:
                            if (editor_state and editor_state->chart_state) {
                                editor_state->chart_state->handle_time_selection_tab(
                                    editor_state->current_exact_beats());
                            }
                            break;

                        // Delete selected notes from the chart and discard
                        // time selection
                        case sf::Keyboard::Delete:
                            if (editor_state) {
                                editor_state->delete_(notificationsQueue);
                            }
                            break;

                        // Arrow keys
                        case sf::Keyboard::Up:
                            if (event.key.shift) {
                                if (editor_state) {
                                    editor_state->volume_up();
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(fmt::format(
                                            "Music Volume : {}%",
                                            editor_state->get_volume() * 10)));
                                }
                            } else {
                                if (editor_state) {
                                    editor_state->move_backwards_in_time();
                                }
                            }
                            break;
                        case sf::Keyboard::Down:
                            if (event.key.shift) {
                                if (editor_state) {
                                    editor_state->volume_down();
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(fmt::format(
                                            "Music Volume : {}%",
                                            editor_state->get_volume() * 10)));
                                }
                            } else {
                                if (editor_state) {
                                    editor_state->move_forwards_in_time();
                                }
                            }
                            break;
                        case sf::Keyboard::Left:
                            if (not ImGui::GetIO().WantTextInput) {
                                if (event.key.shift) {
                                    if (editor_state) {
                                        editor_state->speed_down();
                                        notificationsQueue.push(
                                            std::make_shared<TextNotification>(fmt::format(
                                                "Speed : {}%",
                                                editor_state->get_speed() * 10)));
                                    }
                                } else {
                                    if (editor_state and editor_state->chart_state) {
                                        editor_state->snap = Toolbox::getPreviousDivisor(
                                            240,
                                            editor_state->snap);
                                        notificationsQueue.push(std::make_shared<TextNotification>(fmt::format(
                                            "Snap : {}",
                                            Toolbox::toOrdinal(4 * editor_state->snap))));
                                    }
                                }
                            }
                            break;
                        case sf::Keyboard::Right:
                            if (not ImGui::GetIO().WantTextInput) {
                                if (event.key.shift) {
                                    if (editor_state) {
                                        editor_state->speed_up();
                                        notificationsQueue.push(
                                            std::make_shared<TextNotification>(fmt::format(
                                                "Speed : {}%",
                                                editor_state->get_speed() * 10)));
                                    }
                                } else {
                                    if (editor_state and editor_state->chart_state) {
                                        editor_state->snap = Toolbox::getNextDivisor(
                                            240,
                                            editor_state->snap);
                                        notificationsQueue.push(std::make_shared<TextNotification>(fmt::format(
                                            "Snap : {}",
                                            Toolbox::toOrdinal(4 * editor_state->snap))));
                                    }
                                }
                            }
                            break;

                        /*
                         * F keys
                         */
                        case sf::Keyboard::F1:
                            show_shortcuts_help = not show_shortcuts_help;
                            break;
                        case sf::Keyboard::F3:
                            if (editor_state) {
                                editor_state->toggle_beat_ticks();
                                notificationsQueue.push(
                                    std::make_shared<TextNotification>(
                                        fmt::format(
                                            "Beat tick : {}",
                                            config.sound.beat_tick ? "on" : "off"
                                        )
                                    )
                                );
                            } 
                            break;
                        case sf::Keyboard::F4:
                            if (editor_state) {
                                if (event.key.shift) {
                                    const bool chord = not config.sound.note_clap;
                                    editor_state->play_note_claps(chord);
                                    editor_state->play_chord_claps(chord);
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(
                                            fmt::format(
                                                "Chord tick : {}",
                                                config.sound.distinct_chord_clap ? "on" : "off"
                                            )
                                        )
                                    );
                                } else {
                                    editor_state->toggle_note_claps();
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(
                                            fmt::format(
                                                "Note clap : {}",
                                                config.sound.note_clap ? "on" : "off"
                                            )
                                        )
                                    );
                                }
                            }
                            break;
                        case sf::Keyboard::Space:
                            if (not ImGui::GetIO().WantTextInput) {
                                if (editor_state) {
                                    editor_state->toggle_playback();
                                }
                            }
                            break;
                        case sf::Keyboard::Add:
                            if (editor_state) {
                                editor_state->linear_view.zoom_in();
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Zoom in"));
                            }
                            break;
                        case sf::Keyboard::Subtract:
                            if (editor_state) {
                                editor_state->linear_view.zoom_out();
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Zoom out"));
                            }
                            break;
                        /*
                         * Letter keys, in alphabetical order
                         */
                        case sf::Keyboard::C:
                            if (event.key.control) {
                                if (editor_state and editor_state->chart_state) {
                                    // Let imgui copy/paste work on its own
                                    if (not ImGui::IsAnyItemActive()) {
                                        editor_state->chart_state->copy(notificationsQueue);
                                    }
                                }
                            }
                            break;
                        case sf::Keyboard::F:
                            if (editor_state) {
                                config.playfield.show_free_buttons = not config.playfield.show_free_buttons;
                            }
                        case sf::Keyboard::O:
                            if (event.key.control) {
                                feis::save_ask_open(editor_state, assets_folder, settings_folder, config);
                            }
                            break;
                        case sf::Keyboard::P:
                            if (event.key.shift) {
                                editor_state->show_file_properties = true;
                            }
                            break;
                        case sf::Keyboard::S:
                            if (event.key.control) {
                                feis::force_save(editor_state, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::V:
                            if (event.key.control) {
                                if (editor_state) {
                                    // Let imgui copy/paste work on its own
                                    if (not ImGui::IsAnyItemActive()) {
                                        editor_state->paste(notificationsQueue);
                                    }
                                }
                            }
                            break;
                        case sf::Keyboard::X:
                            if (event.key.control) {
                                if (editor_state) {
                                    // Let imgui copy/paste work on its own
                                    if (not ImGui::IsAnyItemActive()) {
                                        editor_state->cut(notificationsQueue);
                                    }
                                }
                            }
                            break;
                        case sf::Keyboard::Y:
                            if (event.key.control) {
                                if (editor_state) {
                                    editor_state->redo(notificationsQueue);
                                }
                            }
                            break;
                        case sf::Keyboard::Z:
                            if (event.key.control) {
                                if (editor_state) {
                                    editor_state->undo(notificationsQueue);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::KeyReleased:
                    switch (event.key.code) {
                        case sf::Keyboard::F:
                            if (editor_state) {
                                config.playfield.show_free_buttons = not config.playfield.show_free_buttons;
                            }
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

        sf::Time delta = deltaClock.restart();
        ImGui::SFML::Update(window, delta);

        // Audio playback management
        if (editor_state) {
            editor_state->update_visible_notes();
            editor_state->update_music_preview_status();
            if (editor_state->get_status() == sf::SoundSource::Playing) {
                editor_state->previous_playback_position = editor_state->playback_position;
                if (editor_state->has_any_audio()) {
                    editor_state->playback_position =
                        editor_state->get_precise_playback_position();
                } else {
                    editor_state->playback_position = editor_state->current_time()
                        + delta * editor_state->get_pitch();
                }
                if (editor_state->current_time()
                    > editor_state->get_editable_range().end) {
                    editor_state->pause();
                }
            }
        }

        // Drawing
        if (editor_state) {
            window.clear(sf::Color(0, 0, 0));

            if (editor_state->show_history) {
                editor_state->display_history();
            }
            if (editor_state->chart_state and editor_state->show_playfield) {
                editor_state->display_playfield(markers.get_chosen_marker(), markerEndingState);
            }
            if (editor_state->show_playfield_settings) {
                editor_state->display_playfield_settings();
            }
            if (editor_state->show_linear_view) {
                editor_state->display_linear_view();
            }
            if (editor_state->linear_view.shouldDisplaySettings) {
                editor_state->linear_view.display_settings();
            }
            if (editor_state->show_file_properties) {
                editor_state->display_file_properties();
            }
            if (editor_state->show_debug) {
                editor_state->display_debug();
            }
            if (editor_state->show_playback_status) {
                editor_state->display_playback_status();
            }
            if (editor_state->show_timeline) {
                editor_state->display_timeline();
            }
            if (editor_state->show_chart_list) {
                editor_state->display_chart_list();
            }
            if (editor_state->show_new_chart_dialog) {
                auto pair = newChartDialog.display(*editor_state);
                if (pair) {
                    auto& [dif_name, new_chart] = *pair;
                    editor_state->show_new_chart_dialog = false;
                    editor_state->insert_chart_and_push_history(dif_name, new_chart);
                }
            } else {
                newChartDialog.resetValues();
            }
            if (editor_state->show_chart_properties) {
                chartPropertiesDialog.display(*editor_state);
            } else {
                chartPropertiesDialog.should_refresh_values = true;
            }
            if (editor_state->show_sound_settings) {
                editor_state->display_sound_settings();
            }
            if (editor_state->show_editor_settings) {
                editor_state->display_editor_settings();
            }
            if (editor_state->show_sync_menu) {
                editor_state->display_sync_menu();
            }
            if (editor_state->show_bpm_change_menu) {
                editor_state->display_bpm_change_menu();
            }
            if (editor_state->show_timing_kind_menu) {
                editor_state->display_timing_kind_menu();
            }
        } else {
            bg.render(window);
        }

        if (show_shortcuts_help) {
            feis::display_shortcuts_help(show_shortcuts_help);
        }
        if (show_about_menu) {
            feis::display_about_menu(show_about_menu);
        }

        notificationsQueue.display();

        // Main Menu bar drawing
        ImGui::BeginMainMenuBar();
        {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    if (not editor_state) {
                        editor_state.emplace(assets_folder, config);
                    } else {
                        if (editor_state->save_if_needed_and_user_wants_to()
                            != EditorState::SaveOutcome::UserCanceled) {
                            editor_state.emplace(assets_folder, config);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    feis::save_ask_open(editor_state, assets_folder, settings_folder, config);
                }
                if (ImGui::BeginMenu("Recent Files")) {
                    int i = 0;
                    for (const auto& file : Toolbox::getRecentFiles(settings_folder)) {
                        ImGui::PushID(i);
                        if (ImGui::MenuItem(path_to_utf8_encoded_string(file).c_str())) {
                            feis::save_open(editor_state, file, assets_folder, settings_folder, config);
                        }
                        ImGui::PopID();
                        ++i;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Close", "", false, editor_state.has_value())) {
                    feis::save_close(editor_state);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S", false, editor_state.has_value())) {
                    feis::force_save(editor_state, notificationsQueue);
                }
                if (ImGui::MenuItem("Save As", "", false, editor_state.has_value())) {
                    if (editor_state) {
                        if (const auto& path = feis::save_file_dialog()) {
                            editor_state->save(*path);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Properties", "Shift+P", false, editor_state.has_value())) {
                    editor_state->show_file_properties = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit", editor_state.has_value())) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    if (editor_state) {
                        editor_state->undo(notificationsQueue);
                    }
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    if (editor_state) {
                        editor_state->redo(notificationsQueue);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                    if (editor_state) {
                        editor_state->cut(notificationsQueue);
                    }
                }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    if (editor_state and editor_state->chart_state) {
                        editor_state->chart_state->copy(notificationsQueue);
                    }
                }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    if (editor_state) {
                        editor_state->paste(notificationsQueue);
                    }
                }
                if (ImGui::MenuItem("Delete", "Delete")) {
                    if (editor_state) {
                        editor_state->delete_(notificationsQueue);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Chart", editor_state.has_value())) {
                if (ImGui::MenuItem("Chart List")) {
                    editor_state->show_chart_list = true;
                }
                if (ImGui::MenuItem(
                        "Properties##Chart",
                        nullptr,
                        false,
                        editor_state->chart_state.has_value())) {
                    editor_state->show_chart_properties = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("New Chart")) {
                    editor_state->show_new_chart_dialog = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem(
                        "Delete Chart",
                        nullptr,
                        false,
                        editor_state->chart_state.has_value())) {
                    editor_state->erase_chart_and_push_history(
                        editor_state->chart_state->difficulty_name);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Timing", editor_state.has_value())) {
                if (ImGui::MenuItem("Adjust Sync")) {
                    editor_state->show_sync_menu = true;
                }
                if (ImGui::MenuItem("Insert BPM Change")) {
                    editor_state->show_bpm_change_menu = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Timing Kind")) {
                    editor_state->show_timing_kind_menu = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Notes", editor_state.has_value())) {
                if (ImGui::BeginMenu("Mirror")) {
                    if (ImGui::MenuItem("Horizontally")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->mirror_selection_horizontally(notificationsQueue);
                        }
                    }
                    if (ImGui::MenuItem("Vertically")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->mirror_selection_vertically(notificationsQueue);
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Rotate")) {
                    if (ImGui::MenuItem("90° Clockwise")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->rotate_selection_90_clockwise(notificationsQueue);
                        }
                    }
                    if (ImGui::MenuItem("90° Counter-Clockwise")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->rotate_selection_90_counter_clockwise(
                                notificationsQueue);
                        }
                    }
                    if (ImGui::MenuItem("180°")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->rotate_selection_180(notificationsQueue);
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Quantize")) {
                    if (ImGui::MenuItem(fmt::format(
                                            "To snap ({})",
                                            Toolbox::toOrdinal(4 * editor_state->snap))
                                            .c_str())) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->quantize_selection(
                                editor_state->snap,
                                notificationsQueue);
                        }
                    }
                    for (const auto& [snap, color] :
                         config.linear_view.quantization_colors.palette) {
                        feis::ColorDot(color);
                        ImGui::SameLine();
                        if (ImGui::MenuItem(
                                fmt::format("To {}##Notes Quantize", Toolbox::toOrdinal(4 * snap))
                                    .c_str())) {
                            if (editor_state->chart_state.has_value()) {
                                editor_state->chart_state->quantize_selection(snap, notificationsQueue);
                            }
                        }
                    }
                    if (ImGui::BeginMenu("Other")) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("1 /");
                        ImGui::SameLine();
                        static int other_snap = 1;
                        ImGui::SetNextItemWidth(150.f);
                        if (ImGui::InputInt("Beats", &other_snap)) {
                            other_snap = std::max(other_snap, 1);
                        }
                        if (ImGui::Button("Quantize")) {
                            if (editor_state->chart_state.has_value()) {
                                editor_state->chart_state->quantize_selection(other_snap, notificationsQueue);
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View", editor_state.has_value())) {
                if (ImGui::MenuItem("Playfield", nullptr, editor_state->show_playfield)) {
                    editor_state->show_playfield = not editor_state->show_playfield;
                }
                if (ImGui::MenuItem("Linear View", nullptr, editor_state->show_linear_view)) {
                    editor_state->show_linear_view = not editor_state->show_linear_view;
                }
                if (ImGui::MenuItem("Playback Status", nullptr, editor_state->show_playback_status)) {
                    editor_state->show_playback_status = not editor_state->show_playback_status;
                }
                if (ImGui::MenuItem("Timeline", nullptr, editor_state->show_timeline)) {
                    editor_state->show_timeline = not editor_state->show_timeline;
                }
                if (ImGui::MenuItem("Debug", nullptr, editor_state->show_debug)) {
                    editor_state->show_debug = not editor_state->show_debug;
                }
                if (ImGui::MenuItem("History", nullptr, editor_state->show_history)) {
                    editor_state->show_history = not editor_state->show_history;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings", editor_state.has_value())) {
                if (ImGui::MenuItem("Playfield##Settings")) {
                    editor_state->show_playfield_settings = true;
                }
                if (ImGui::MenuItem("Sound")) {
                    editor_state->show_sound_settings = true;
                }
                if (ImGui::MenuItem("Linear View")) {
                    editor_state->linear_view.shouldDisplaySettings = true;
                }
                if (ImGui::MenuItem("Editor")) {
                    editor_state->show_editor_settings = true;
                }
                if (ImGui::BeginMenu("Marker")) {
                    int i = 0;
                    std::for_each(
                        markers.cbegin(),
                        markers.cend(),
                        [&](const auto& it){
                            const auto& [path, opt_preview] = it;
                            ImGui::PushID(path.c_str());
                            bool clicked = false;
                            if (opt_preview) {
                                clicked = ImGui::ImageButton(*opt_preview, {100, 100});
                            } else {
                                clicked = ImGui::Button(path_to_utf8_encoded_string(path).c_str(), {100, 100});
                            }
                            if (clicked) {
                                markers.load_marker(path);
                            }
                            ImGui::PopID();
                            i++;
                            if (i % 4 != 0) {
                                ImGui::SameLine();
                            }
                        }
                    );
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Marker Ending State")) {
                    for (const auto& [judgement, name] : judgement_to_name) {
                        if (const auto& marker = markers.get_chosen_marker()) {
                            const auto preview = (*marker)->judgement_preview(judgement);
                            if (ImGui::ImageButton(preview, {100, 100})) {
                                markerEndingState = judgement;
                            }
                            ImGui::SameLine();
                            ImGui::TextUnformatted(name.c_str());
                        } else {
                            if (ImGui::Button((name+"##Marker Ending State").c_str())) {
                                markerEndingState = judgement;
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Shortcuts", "F1")) {
                    show_shortcuts_help = true;
                }
                if (ImGui::MenuItem("About")) {
                    show_about_menu = true;
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
        ImGui::SFML::Render(window);
        window.display();
        markers.update();
    }

    ImGui::SFML::Shutdown();
    return 0;
}