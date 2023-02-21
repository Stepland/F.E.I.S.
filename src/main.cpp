#include <SFML/Window/Keyboard.hpp>
#include <exception>
#include <string>
#include <filesystem>
#include <variant>

#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <SFML/Audio/SoundFileFactory.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include <tinyfiledialogs.h>
#include <whereami++.hpp>

#include "src/custom_sfml_audio/synced_sound_streams.hpp"
#include "widgets/blank_screen.hpp"
#include "chart_state.hpp"
#include "config.hpp"
#include "editor_state.hpp"
#include "file_dialogs.hpp"
#include "history_item.hpp"
#include "marker.hpp"
#include "mp3_reader.hpp"
#include "notifications_queue.hpp"
#include "utf8_sfml.hpp"
#include "utf8_strings.hpp"
#include "widgets/waveform_view.hpp"

int main() {
    // TODO : Make the playfield not appear when there's no chart selected

    // extend SFML to be able to read mp3's
    sf::SoundFileFactory::registerReader<sf::priv::SoundFileReaderMp3>();

    auto executable_folder = to_path(whereami::executable_dir());
    auto assets_folder = executable_folder / "assets";
    auto settings_folder = executable_folder / "settings";

    config::Config config{settings_folder};

    sf::RenderWindow window(sf::VideoMode(800, 600), "FEIS");
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    ImGui::SFML::Init(window, false);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 5.f;

    auto font_path = assets_folder / "fonts" / "NotoSans-Medium.ttf";
    if (not std::filesystem::exists(font_path)) {
        tinyfd_messageBox(
            "Error",
            ("Could not open "+to_utf8_encoded_string(font_path)).c_str(),
            "ok",
            "error",
            1
        );
        return -1;
    }
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->Clear();
    IO.Fonts->AddFontFromFileTTF(
        to_utf8_encoded_string(assets_folder / "fonts" / "NotoSans-Medium.ttf").c_str(),
        16.f
    );
    ImGui::SFML::UpdateFontTexture();

    IO.ConfigWindowsMoveFromTitleBarOnly = true;

    // Loading markers preview
    std::map<std::filesystem::path, sf::Texture> markerPreviews;
    for (const auto& folder :
         std::filesystem::directory_iterator(assets_folder / "textures" / "markers")) {
        if (folder.is_directory()) {
            feis::LoadFromPathMixin<sf::Texture> markerPreview;
            markerPreview.load_from_path(folder.path() / "ma15.png");
            markerPreview.setSmooth(true);
            markerPreviews.insert({folder, markerPreview});
        }
    }

    std::optional<Marker> default_marker_opt;
    try {
        default_marker_opt = first_available_marker_in(assets_folder);
    } catch (const std::exception& e) {
        fmt::print("Couldn't load any marker folder, aborting");
        return -1;
    }
    Marker& default_marker = *default_marker_opt;

    std::optional<Marker> marker_opt;
    if (config.marker.folder) {
        try {
            marker_opt = Marker(*config.marker.folder);
        } catch (const std::exception& e) {
            fmt::print("Failed to load marker from preferences");
            marker_opt = default_marker_opt;
            config.marker.folder = marker_opt->get_folder();
        }
    } else {
        marker_opt = default_marker_opt;
        config.marker.folder = marker_opt->get_folder();
    }
    Marker& marker = *marker_opt;
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
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            switch (event.type) {
                case sf::Event::Closed:
                    if (editor_state) {
                        if (editor_state->save_if_needed_and_user_wants_to() != EditorState::SaveOutcome::UserCanceled) {
                            window.close();
                        }
                    } else {
                        window.close();
                    }
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View(
                        sf::FloatRect(0, 0, event.size.width, event.size.height)));
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
                                    editor_state->current_exact_beats()
                                );
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
                                            editor_state->get_volume() * 10
                                        ))
                                    );
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
                                            editor_state->get_volume() * 10
                                        ))
                                    );
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
                                        notificationsQueue.push(std::make_shared<TextNotification>(fmt::format(
                                            "Speed : {}%",
                                            editor_state->get_speed() * 10
                                        )));
                                    }
                                } else {
                                    if (editor_state and editor_state->chart_state) {
                                        editor_state->snap = Toolbox::getPreviousDivisor(240, editor_state->snap);
                                        notificationsQueue.push(
                                            std::make_shared<TextNotification>(fmt::format(
                                                "Snap : {}",
                                                Toolbox::toOrdinal(4 * editor_state->snap)
                                            ))
                                        );
                                    }
                                }
                            }
                            break;
                        case sf::Keyboard::Right:
                            if (not ImGui::GetIO().WantTextInput) {
                                if (event.key.shift) {
                                    if (editor_state) {
                                        editor_state->speed_up();
                                        notificationsQueue.push(std::make_shared<TextNotification>(fmt::format(
                                            "Speed : {}%",
                                            editor_state->get_speed() * 10
                                        )));
                                    }
                                } else {
                                    if (editor_state and editor_state->chart_state) {
                                        editor_state->snap = Toolbox::getNextDivisor(240, editor_state->snap);
                                        notificationsQueue.push(
                                            std::make_shared<TextNotification>(fmt::format(
                                                "Snap : {}",
                                                Toolbox::toOrdinal(4 * editor_state->snap)
                                            ))
                                        );
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
                            /*
                            if (beatTick.toggle()) {
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Beat tick : on"));
                            } else {
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Beat tick : off"));
                            }
                            */
                            break;
                        case sf::Keyboard::F4:
                            /*
                            if (event.key.shift) {
                                if (chordTick.toggle()) {
                                    noteTick.shouldPlay = true;
                                    notificationsQueue.push(std::make_shared<TextNotification>(
                                        "Note+Chord tick : on"));
                                } else {
                                    noteTick.shouldPlay = false;
                                    notificationsQueue.push(std::make_shared<TextNotification>(
                                        "Note+Chord tick : off"));
                                }
                            } else {
                                if (noteTick.toggle()) {
                                    notificationsQueue.push(std::make_shared<TextNotification>(
                                        "Note tick : on"));
                                } else {
                                    notificationsQueue.push(std::make_shared<TextNotification>(
                                        "Note tick : off"));
                                }
                            }
                            */
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
                                std::visit([](auto& vv){vv.zoom_in();}, editor_state->vertical_view);
                                notificationsQueue.push(std::make_shared<TextNotification>("Zoom in"));
                            }
                            break;
                        case sf::Keyboard::Subtract:
                            if (editor_state) {
                                std::visit([](auto& vv){vv.zoom_out();}, editor_state->vertical_view);
                                notificationsQueue.push(std::make_shared<TextNotification>("Zoom out"));
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
                                config.editor.show_free_buttons = not config.editor.show_free_buttons;
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
                                config.editor.show_free_buttons = not config.editor.show_free_buttons;
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
                    editor_state->playback_position = editor_state->get_precise_playback_position();
                } else {
                    editor_state->playback_position = editor_state->current_time() + delta * editor_state->get_pitch();
                }
                if (editor_state->current_time() > editor_state->get_editable_range().end) {
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
            if (editor_state->show_playfield) {
                editor_state->display_playfield(marker, markerEndingState);
            }
            if (editor_state->show_linear_view) {
                editor_state->display_linear_view();
            }
            auto display_vertical_view_settings = VariantVisitor {
                [](char) { return "char"; },
                [](int) { return "int"; },
                [](auto) { return "unknown type"; },
            };
            if (editor_state->vertical_view_.shouldDisplaySettings) {
                editor_state->vertical_view_.display_settings();
            }
            if (editor_state->show_file_properties) {
                editor_state->display_file_properties();
            }
            if (editor_state->show_status) {
                editor_state->display_status();
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
            if (editor_state->show_timing_menu) {
                editor_state->display_timing_menu();
            }
            if (editor_state->waveform_view) {
                editor_state->waveform_view->draw(editor_state->current_time());
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
                        if (editor_state->save_if_needed_and_user_wants_to() != EditorState::SaveOutcome::UserCanceled) {
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
                        if (ImGui::MenuItem(file.c_str())) {
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
                    editor_state->erase_chart_and_push_history(editor_state->chart_state->difficulty_name);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Timing", editor_state.has_value())) {
                if (ImGui::MenuItem("Adjust Timing")) {
                    editor_state->show_timing_menu = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Notes", editor_state.has_value())) {
                if (ImGui::BeginMenu("Mirror")) {
                    if (ImGui::MenuItem("Horizontally")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->mirror_selected_horizontally(notificationsQueue);
                        }
                    }
                    if (ImGui::MenuItem("Vertically")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->mirror_selected_vertically(notificationsQueue);
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Rotate")) {
                    if (ImGui::MenuItem("90° Clockwise")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->rotate_selected_90_clockwise(notificationsQueue);
                        }
                    }
                    if (ImGui::MenuItem("90° Counter-Clockwise")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->rotate_selected_90_counter_clockwise(notificationsQueue);
                        }
                    }
                    if (ImGui::MenuItem("180°")) {
                        if (editor_state->chart_state.has_value()) {
                            editor_state->chart_state->rotate_selected_180(notificationsQueue);
                        }
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
                if (ImGui::MenuItem("Editor Status", nullptr, editor_state->show_status)) {
                    editor_state->show_status = not editor_state->show_status;
                }
                if (ImGui::MenuItem("History", nullptr, editor_state->show_history)) {
                    editor_state->show_history = not editor_state->show_history;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings", editor_state.has_value())) {
                if (ImGui::MenuItem("Sound")) {
                    editor_state->show_sound_settings = true;
                }
                if (ImGui::MenuItem("Linear View")) {
                    editor_state->vertical_view_.shouldDisplaySettings = true;
                }
                if (ImGui::MenuItem("Editor")) {
                    editor_state->show_editor_settings = true;
                }
                if (ImGui::BeginMenu("Marker")) {
                    int i = 0;
                    for (auto& tuple : markerPreviews) {
                        ImGui::PushID(tuple.first.c_str());
                        if (ImGui::ImageButton(tuple.second, {100, 100})) {
                            try {
                                marker = Marker(tuple.first);
                                config.marker.folder = marker.get_folder();
                            } catch (const std::exception& e) {
                                tinyfd_messageBox(
                                    "Error",
                                    e.what(),
                                    "ok",
                                    "error",
                                    1);
                                marker = default_marker;
                                config.marker.folder = marker.get_folder();
                            }
                        }
                        ImGui::PopID();
                        i++;
                        if (i % 4 != 0) {
                            ImGui::SameLine();
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Marker Ending State")) {
                    for (const auto& [judgement, name] : judgement_to_name) {
                        if (ImGui::ImageButton(marker.preview(judgement), {100, 100})) {
                            markerEndingState = judgement;
                        }
                        ImGui::SameLine();
                        ImGui::TextUnformatted(name.c_str());
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
    }

    ImGui::SFML::Shutdown();
    return 0;
}