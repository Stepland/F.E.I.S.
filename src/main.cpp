#define IMGUI_USER_CONFIG "imconfig-SFML.h"

#include <filesystem>
#include <variant>

#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <SFML/Graphics.hpp>
#include <tinyfiledialogs.h>
#include <whereami++.hpp>

#include "chart_state.hpp"
#include "editor_state.hpp"
#include "history_actions.hpp"
#include "notifications_queue.hpp"
#include "preferences.hpp"
#include "sound_effect.hpp"
#include "widgets/blank_screen.hpp"

int main() {
    // TODO : Make the playfield not appear when there's no chart selected
    // TODO : Make the linear preview display the end of the chart
    // TODO : Make the linear preview timebar height movable

    auto executable_folder = std::filesystem::path{whereami::executable_dir()};
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
    MarkerEndingState markerEndingState = preferences.markerEndingState;

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
                        if (editor_state->save_if_needed() != EditorState::SaveOutcome::UserCanceled) {
                            window.close();
                        }
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
                                    auto new_note = make_long_note(
                                        *editor_state->chart_state->long_note_being_created
                                    );
                                    better::Notes new_notes;
                                    new_notes.insert(new_note);
                                    editor_state->chart_state->chart.notes.insert(new_note);
                                    editor_state->chart_state->long_note_being_created.reset();
                                    editor_state->chart_state->creating_long_note = false;
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
                                    editor_state->current_snaped_beats()
                                );
                            }
                            break;

                        // Delete selected notes from the chart and discard
                        // time selection
                        case sf::Keyboard::Delete:
                            Edit::delete_(editor_state, notificationsQueue);
                            break;

                        // Arrow keys
                        case sf::Keyboard::Up:
                            if (event.key.shift) {
                                if (editor_state and editor_state->music_state) {
                                    editor_state->music_state->volume_up();
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(fmt::format(
                                            "Music Volume : {}%",
                                            editor_state->music_state->volume * 10
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
                                if (editor_state and editor_state->music_state) {
                                    editor_state->music_state->volume_down();
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(fmt::format(
                                            "Music Volume : {}%",
                                            editor_state->music_state->volume * 10
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
                                if (editor_state and editor_state->music_state) {
                                    editor_state->music_state->speed_down();
                                    notificationsQueue.push(std::make_shared<TextNotification>(fmt::format(
                                        "Speed : {}%",
                                        editor_state->music_state->speed * 10
                                    )));
                                }
                            } else {
                                if (editor_state and editor_state->chart_state) {
                                    editor_state->snap = Toolbox::getPreviousDivisor(240, editor_state->snap);
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(fmt::format(
                                            "Snap : {}%",
                                            Toolbox::toOrdinal(4 * editor_state->snap)
                                        ))
                                    );
                                }
                            }
                            break;
                        case sf::Keyboard::Right:
                            if (event.key.shift) {
                                if (editor_state and editor_state->music_state) {
                                    editor_state->musicSpeedUp();
                                    std::stringstream ss;
                                    ss << "Speed : " << editor_state->musicSpeed * 10 << "%";
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(ss.str()));
                                }
                            } else {
                                if (editor_state and editor_state->chart) {
                                    editor_state->snap = Toolbox::getNextDivisor(
                                        editor_state->chart->ref.getResolution(),
                                        editor_state->snap);
                                    std::stringstream ss;
                                    ss << "Snap : "
                                       << Toolbox::toOrdinal(4 * editor_state->snap);
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>(ss.str()));
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
                                editor_state->linearView.zoom_in();
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Zoom in"));
                            }
                            break;
                        case sf::Keyboard::Subtract:
                            if (editor_state) {
                                editor_state->linearView.zoom_out();
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Zoom out"));
                            }
                            break;
                        /*
                         * Letter keys, in alphabetical order
                         */
                        case sf::Keyboard::C:
                            if (event.key.control) {
                                Edit::copy(editor_state, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::O:
                            if (event.key.control) {
                                if (feis::saveOrCancel(editor_state)) {
                                    feis::open(editor_state, assets_folder, settings_folder);
                                }
                            }
                            break;
                        case sf::Keyboard::P:
                            if (event.key.shift) {
                                editor_state->showProperties = true;
                            }
                            break;
                        case sf::Keyboard::S:
                            if (event.key.control) {
                                feis::save(*editor_state);
                                notificationsQueue.push(std::make_shared<TextNotification>(
                                    "Saved file"));
                            }
                            break;
                        case sf::Keyboard::V:
                            if (event.key.control) {
                                Edit::paste(editor_state, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::X:
                            if (event.key.control) {
                                Edit::cut(editor_state, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::Y:
                            if (event.key.control) {
                                Edit::redo(editor_state, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::Z:
                            if (event.key.control) {
                                Edit::undo(editor_state, notificationsQueue);
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
            if (editor_state->chart_state) {
                editor_state->chart_state->update_visible_notes();
            }
            if (editor_state->playing) {
                editor_state->previousPos = editor_state->playbackPosition;
                editor_state->playbackPosition += delta * (editor_state->musicSpeed / 10.f);
                if (editor_state->music) {
                    switch (editor_state->music->getStatus()) {
                        case sf::Music::Stopped:
                        case sf::Music::Paused:
                            if (editor_state->playbackPosition.asSeconds() >= 0
                                and editor_state->playbackPosition
                                    < editor_state->music->getDuration()) {
                                editor_state->music->setPlayingOffset(editor_state->playbackPosition);
                                editor_state->music->play();
                            }
                            break;
                        case sf::Music::Playing:
                            editor_state->playbackPosition =
                                editor_state->music->getPrecisePlayingOffset();
                            break;
                        default:
                            break;
                    }
                }
                if (beatTick.shouldPlay) {
                    auto previous_tick = static_cast<int>(editor_state->ticks_at(
                        editor_state->previousPos.asSeconds()));
                    auto current_tick = static_cast<int>(editor_state->ticks_at(
                        editor_state->playbackPosition.asSeconds()));
                    if (previous_tick / editor_state->getResolution()
                        != current_tick / editor_state->getResolution()) {
                        beatTick.play();
                    }
                }
                if (noteTick.shouldPlay) {
                    int note_count = 0;
                    for (auto note : editor_state->visibleNotes) {
                        float noteTiming = editor_state->getSecondsAt(note.getTiming());
                        if (noteTiming >= editor_state->previousPos.asSeconds()
                            and noteTiming <= editor_state->playbackPosition.asSeconds()) {
                            note_count++;
                        }
                    }
                    if (chordTick.shouldPlay) {
                        if (note_count > 1) {
                            chordTick.play();
                        } else if (note_count == 1) {
                            noteTick.play();
                        }
                    } else if (note_count >= 1) {
                        noteTick.play();
                    }
                }

                if (editor_state->playbackPosition > editor_state->getPreviewEnd()) {
                    editor_state->playing = false;
                    editor_state->playbackPosition = editor_state->getPreviewEnd();
                }
            } else {
                if (editor_state->music) {
                    if (editor_state->music->getStatus() == sf::Music::Playing) {
                        editor_state->music->pause();
                    }
                }
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
            if (editor_state->linearView.shouldDisplaySettings) {
                editor_state->linearView.displaySettings();
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
                editor_state->display_chart_list(assets_folder);
            }
            if (editor_state->showNewChartDialog) {
                std::optional<Chart> c = newChartDialog.display(*editor_state);
                if (c) {
                    editor_state->showNewChartDialog = false;
                    if (editor_state->song.Charts.try_emplace(c->dif_name, *c).second) {
                        editor_state->chart.emplace(editor_state->song.Charts.at(c->dif_name), assets_folder);
                    }
                }
            } else {
                newChartDialog.resetValues();
            }
            if (editor_state->showChartProperties) {
                chartPropertiesDialog.display(*editor_state, assets_folder);
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
                    bool user_canceled = false;
                    if (editor_state) {
                        switch (editor_state->ask_if_user_wants_to_save()) {
                            case UserWantsToSave::Yes:
                                const auto path = editor_state->ask_for_save_path_if_needed();
                                if (path) {
                                    editor_state->save(*path);
                                } else {
                                    user_canceled = true;
                                }
                                break;
                            case UserWantsToSave::No:
                            case UserWantsToSave::DidNotDisplayDialog: // Already saved
                                break;
                            case UserWantsToSave::Cancel:
                                user_canceled = true;
                                break;
                        }
                    }
                    if (not user_canceled) {
                        editor_state.emplace(assets_folder);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    if (feis::saveOrCancel(editor_state)) {
                        feis::open(editor_state, assets_folder, settings_folder);
                    }
                }
                if (ImGui::BeginMenu("Recent Files")) {
                    int i = 0;
                    for (const auto& file : Toolbox::getRecentFiles(settings_folder)) {
                        ImGui::PushID(i);
                        if (ImGui::MenuItem(file.c_str())) {
                            if (feis::saveOrCancel(editor_state)) {
                                feis::openFromFile(editor_state, file, assets_folder, settings_folder);
                            }
                        }
                        ImGui::PopID();
                        ++i;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Close", "", false, editor_state.has_value())) {
                    if (feis::saveOrCancel(editor_state)) {
                        editor_state.reset();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S", false, editor_state.has_value())) {
                    feis::save(*editor_state);
                }
                if (ImGui::MenuItem("Save As", "", false, editor_state.has_value())) {
                    char const* options[1] = {"*.memon"};
                    const char* _filepath(
                        tinyfd_saveFileDialog("Save File", nullptr, 1, options, nullptr));
                    if (_filepath != nullptr) {
                        std::filesystem::path filepath(_filepath);
                        try {
                            editor_state->song.saveAsMemon(filepath);
                        } catch (const std::exception& e) {
                            tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
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
                    Edit::undo(editor_state, notificationsQueue);
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    Edit::redo(editor_state, notificationsQueue);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                    Edit::cut(editor_state, notificationsQueue);
                }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    Edit::copy(editor_state, notificationsQueue);
                }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    Edit::paste(editor_state, notificationsQueue);
                }
                if (ImGui::MenuItem("Delete", "Delete")) {
                    Edit::delete_(editor_state, notificationsQueue);
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
                        editor_state->chart.has_value())) {
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
                        editor_state->chart.has_value())) {
                    editor_state->song.Charts.erase(editor_state->chart->ref.dif_name);
                    editor_state->chart.reset();
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
                    editor_state->linearView.shouldDisplaySettings = true;
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
                    for (auto& m : Markers::markerStatePreviews) {
                        if (ImGui::ImageButton(marker.getTextures().at(m.textureName), {100, 100})) {
                            markerEndingState = m.state;
                            preferences.markerEndingState = m.state;
                        }
                        ImGui::SameLine();
                        ImGui::TextUnformatted(m.printName.c_str());
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