#include <string>
#include <filesystem>
#include <variant>

#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <SFML/Audio/SoundFileFactory.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include <tinyfiledialogs.h>
#include <whereami++.hpp>

#include "chart_state.hpp"
#include "editor_state.hpp"
#include "file_dialogs.hpp"
#include "history_item.hpp"
#include "marker.hpp"
#include "mp3_reader.hpp"
#include "notifications_queue.hpp"
#include "preferences.hpp"
#include "sound_effect.hpp"
#include "src/custom_sfml_audio/synced_sound_streams.hpp"
#include "widgets/blank_screen.hpp"

int main() {
    // TODO : Make the playfield not appear when there's no chart selected
    // TODO : Make the linear preview display the end of the chart
    // TODO : Make the linear preview timebar height movable

    // extend SFML to be able to read mp3's
    sf::SoundFileFactory::registerReader<sf::priv::SoundFileReaderMp3>();

    auto executable_folder = std::filesystem::u8path(whereami::executable_dir());
    auto assets_folder = executable_folder / "assets";
    auto settings_folder = executable_folder / "settings";

    sf::RenderWindow window(sf::VideoMode(800, 600), "FEIS");
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window, false);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 5.f;

    auto font_path = assets_folder / "fonts" / "NotoSans-Medium.ttf";
    if (not std::filesystem::exists(font_path)) {
        tinyfd_messageBox(
            "Error",
            ("Could not open "+font_path.string()).c_str(),
            "ok",
            "error",
            1
        );
        return -1;
    }
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->Clear();
    IO.Fonts->AddFontFromFileTTF(
        (assets_folder / "fonts" / "NotoSans-Medium.ttf").c_str(),
        16.f);
    ImGui::SFML::UpdateFontTexture();

    SoundEffect beatTick {assets_folder / "sounds" / "beat.wav"};
    SoundEffect noteTick {assets_folder / "sounds" / "note.wav"};
    SoundEffect chordTick {assets_folder / "sounds" / "chord.wav"};

    // Loading markers preview
    std::map<std::filesystem::path, sf::Texture> markerPreviews;
    for (const auto& folder :
         std::filesystem::directory_iterator(assets_folder / "textures" / "markers")) {
        if (folder.is_directory()) {
            sf::Texture markerPreview;
            markerPreview.loadFromFile((folder.path() / "ma15.png").string());
            markerPreview.setSmooth(true);
            markerPreviews.insert({folder, markerPreview});
        }
    }

    Preferences preferences{assets_folder, settings_folder};

    Marker defaultMarker = Marker(preferences.marker);
    Marker& marker = defaultMarker;
    Judgement& markerEndingState = preferences.marker_ending_state;

    BlankScreen bg{assets_folder};
    std::optional<EditorState> editor_state;
    NotificationsQueue notificationsQueue;
    feis::NewChartDialog newChartDialog;
    feis::ChartPropertiesDialog chartPropertiesDialog;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            switch (event.type) {
                case sf::Event::Closed:
                    preferences.save();
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
                            if (editor_state and editor_state->chart_state) {
                                if (editor_state->chart_state->long_note_being_created) {
                                    auto new_note = make_linear_view_long_note_dummy(
                                        *editor_state->chart_state->long_note_being_created,
                                        editor_state->get_snap_step()
                                    );
                                    better::Notes new_notes;
                                    new_notes.insert(new_note);
                                    const auto& overwritten = (
                                        editor_state
                                        ->chart_state
                                        ->chart
                                        .notes
                                        .overwriting_insert(new_note)
                                    );
                                    editor_state->chart_state->long_note_being_created.reset();
                                    editor_state->chart_state->creating_long_note = false;
                                    if (not overwritten.empty()) {
                                        editor_state->chart_state->history.push(
                                            std::make_shared<RemoveNotes>(overwritten)
                                        );
                                    }
                                    editor_state->chart_state->history.push(
                                        std::make_shared<AddNotes>(new_notes)
                                    );
                                }
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
                            if (editor_state and editor_state->chart_state) {
                                if (editor_state->chart_state->time_selection) {
                                    editor_state->chart_state->time_selection.reset();
                                } else if (not editor_state->chart_state->selected_notes.empty()) {
                                    editor_state->chart_state->selected_notes.clear();
                                }
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
                            if (editor_state and editor_state->chart_state) {
                                editor_state->chart_state->delete_(notificationsQueue);
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
                            break;
                        case sf::Keyboard::Right:
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
                            break;

                        /*
                         * F keys
                         */
                        case sf::Keyboard::F3:
                            if (beatTick.toggle()) {
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Beat tick : on"));
                            } else {
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Beat tick : off"));
                            }
                            break;
                        case sf::Keyboard::F4:
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
                            break;
                        case sf::Keyboard::Space:
                            if (not ImGui::GetIO().WantTextInput) {
                                if (editor_state) {
                                    editor_state->playing = not editor_state->playing;
                                }
                            }
                            break;
                        case sf::Keyboard::Add:
                            if (editor_state) {
                                editor_state->linear_view.zoom_in();
                                notificationsQueue.push(std::make_shared<TextNotification>("Zoom in"));
                            }
                            break;
                        case sf::Keyboard::Subtract:
                            if (editor_state) {
                                editor_state->linear_view.zoom_out();
                                notificationsQueue.push(std::make_shared<TextNotification>("Zoom out"));
                            }
                            break;
                        /*
                         * Letter keys, in alphabetical order
                         */
                        case sf::Keyboard::C:
                            if (event.key.control) {
                                if (editor_state and editor_state->chart_state) {
                                    editor_state->chart_state->copy(notificationsQueue);
                                }
                            }
                            break;
                        case sf::Keyboard::O:
                            if (event.key.control) {
                                feis::save_ask_open(editor_state, assets_folder, settings_folder);
                            }
                            break;
                        case sf::Keyboard::P:
                            if (event.key.shift) {
                                editor_state->showProperties = true;
                            }
                            break;
                        case sf::Keyboard::S:
                            if (event.key.control) {
                                feis::save(editor_state, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::V:
                            if (event.key.control) {
                                if (editor_state and editor_state->chart_state) {
                                    editor_state->chart_state->paste(
                                        editor_state->current_snaped_beats(),
                                        notificationsQueue
                                    );
                                }
                            }
                            break;
                        case sf::Keyboard::X:
                            if (event.key.control) {
                                if (editor_state and editor_state->chart_state) {
                                    editor_state->chart_state->cut(notificationsQueue);
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
                default:
                    break;
            }
        }

        sf::Time delta = deltaClock.restart();
        ImGui::SFML::Update(window, delta);

        // Audio playback management
        if (editor_state) {
            editor_state->update_visible_notes();
            if (editor_state->playing) {
                editor_state->previous_playback_position = editor_state->playback_position;
                editor_state->playback_position = editor_state->current_time() + delta * (editor_state->get_speed() / 10.f);
                switch (editor_state->get_status()) {
                    case sf::Music::Stopped:
                    case sf::Music::Paused:
                        editor_state->set_playback_position(editor_state->current_time());
                        editor_state->play();
                        break;
                    case sf::Music::Playing:
                        editor_state->playback_position = editor_state->get_playback_position();
                        break;
                    default:
                        break;
                }
                if (beatTick.shouldPlay) {
                    const auto previous_beat = editor_state->previous_exact_beats();
                    const auto current_beat = editor_state->current_exact_beats();
                    if (floor_fraction(previous_beat) != floor_fraction(current_beat)) {
                        beatTick.play();
                    }
                }
                if (editor_state->current_time() > editor_state->get_editable_range().end) {
                    editor_state->playing = false;
                    editor_state->playback_position = editor_state->get_editable_range().end;
                }
            } else if (editor_state->get_status() == sf::SoundSource::Playing) {
                editor_state->pause();
            }
        }

        // Drawing
        if (editor_state) {
            window.clear(sf::Color(0, 0, 0));

            if (editor_state->showHistory) {
                editor_state->chart_state->history.display();
            }
            if (editor_state->showPlayfield) {
                editor_state->display_playfield(marker, markerEndingState);
            }
            if (editor_state->showLinearView) {
                editor_state->display_linear_view();
            }
            if (editor_state->linear_view.shouldDisplaySettings) {
                editor_state->linear_view.display_settings();
            }
            if (editor_state->showProperties) {
                editor_state->display_properties();
            }
            if (editor_state->showStatus) {
                editor_state->display_status();
            }
            if (editor_state->showPlaybackStatus) {
                editor_state->display_playback_status();
            }
            if (editor_state->showTimeline) {
                editor_state->display_timeline();
            }
            if (editor_state->showChartList) {
                editor_state->display_chart_list();
            }
            if (editor_state->showNewChartDialog) {
                auto pair = newChartDialog.display(*editor_state);
                if (pair) {
                    auto& [dif_name, new_chart] = *pair;
                    editor_state->showNewChartDialog = false;
                    if (editor_state->song.charts.try_emplace(dif_name, new_chart).second) {
                        editor_state->open_chart(dif_name);
                    }
                }
            } else {
                newChartDialog.resetValues();
            }
            if (editor_state->showChartProperties) {
                chartPropertiesDialog.display(*editor_state);
            } else {
                chartPropertiesDialog.should_refresh_values = true;
            }

            if (editor_state->showSoundSettings) {
                ImGui::Begin("Sound Settings", &editor_state->showSoundSettings, ImGuiWindowFlags_AlwaysAutoResize);
                {
                    if (ImGui::TreeNode("Beat Tick")) {
                        beatTick.displayControls();
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Note Tick")) {
                        noteTick.displayControls();
                        ImGui::Checkbox("Chord sound", &chordTick.shouldPlay);
                        ImGui::TreePop();
                    }
                }
                ImGui::End();
            }

        } else {
            bg.render(window);
        }

        notificationsQueue.display();

        // Main Menu bar drawing
        ImGui::BeginMainMenuBar();
        {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    if (not editor_state) {
                        editor_state.emplace(assets_folder);
                    } else {
                        if (editor_state->save_if_needed_and_user_wants_to() != EditorState::SaveOutcome::UserCanceled) {
                            editor_state.emplace(assets_folder);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    feis::save_ask_open(editor_state, assets_folder, settings_folder);
                }
                if (ImGui::BeginMenu("Recent Files")) {
                    int i = 0;
                    for (const auto& file : Toolbox::getRecentFiles(settings_folder)) {
                        ImGui::PushID(i);
                        if (ImGui::MenuItem(file.c_str())) {
                            feis::save_open(editor_state, file, assets_folder, settings_folder);
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
                    feis::save(editor_state, notificationsQueue);
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
                    editor_state->showProperties = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
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
                    if (editor_state and editor_state->chart_state) {
                        editor_state->chart_state->cut(notificationsQueue);
                    }
                }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    if (editor_state and editor_state->chart_state) {
                        editor_state->chart_state->copy(notificationsQueue);
                    }
                }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    if (editor_state and editor_state->chart_state) {
                        editor_state->chart_state->paste(
                            editor_state->current_snaped_beats(),
                            notificationsQueue
                        );
                    }
                }
                if (ImGui::MenuItem("Delete", "Delete")) {
                    if (editor_state and editor_state->chart_state) {
                        editor_state->chart_state->delete_(notificationsQueue);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Chart", editor_state.has_value())) {
                if (ImGui::MenuItem("Chart List")) {
                    editor_state->showChartList = true;
                }
                if (ImGui::MenuItem(
                        "Properties##Chart",
                        nullptr,
                        false,
                        editor_state->chart_state.has_value())) {
                    editor_state->showChartProperties = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("New Chart")) {
                    editor_state->showNewChartDialog = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem(
                        "Delete Chart",
                        nullptr,
                        false,
                        editor_state->chart_state.has_value())) {
                    editor_state->song.charts.erase(editor_state->chart_state->difficulty_name);
                    editor_state->chart_state.reset();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View", editor_state.has_value())) {
                if (ImGui::MenuItem("Playfield", nullptr, editor_state->showPlayfield)) {
                    editor_state->showPlayfield = not editor_state->showPlayfield;
                }
                if (ImGui::MenuItem("Linear View", nullptr, editor_state->showLinearView)) {
                    editor_state->showLinearView = not editor_state->showLinearView;
                }
                if (ImGui::MenuItem("Playback Status", nullptr, editor_state->showPlaybackStatus)) {
                    editor_state->showPlaybackStatus = not editor_state->showPlaybackStatus;
                }
                if (ImGui::MenuItem("Timeline", nullptr, editor_state->showTimeline)) {
                    editor_state->showTimeline = not editor_state->showTimeline;
                }
                if (ImGui::MenuItem("Editor Status", nullptr, editor_state->showStatus)) {
                    editor_state->showStatus = not editor_state->showStatus;
                }
                if (ImGui::MenuItem("History", nullptr, editor_state->showHistory)) {
                    editor_state->showHistory = not editor_state->showHistory;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings", editor_state.has_value())) {
                if (ImGui::MenuItem("Sound")) {
                    editor_state->showSoundSettings = true;
                }
                if (ImGui::MenuItem("Linear View")) {
                    editor_state->linear_view.shouldDisplaySettings = true;
                }
                if (ImGui::BeginMenu("Marker")) {
                    int i = 0;
                    for (auto& tuple : markerPreviews) {
                        ImGui::PushID(tuple.first.c_str());
                        if (ImGui::ImageButton(tuple.second, {100, 100})) {
                            try {
                                marker = Marker(tuple.first);
                                preferences.marker = tuple.first.string();
                            } catch (const std::exception& e) {
                                tinyfd_messageBox(
                                    "Error",
                                    e.what(),
                                    "ok",
                                    "error",
                                    1);
                                marker = defaultMarker;
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
                    for (const auto& [judgement, name] : marker_state_previews) {
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
        }
        ImGui::EndMainMenuBar();

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}