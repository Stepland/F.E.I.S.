#include <SFML/Graphics.hpp>
#include <filesystem>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <tinyfiledialogs.h>
#include <variant>
#include <whereami++.hpp>

#include "editor_state.hpp"
#include "editor_state_actions.hpp"
#include "notifications_queue.hpp"
#include "preferences.hpp"
#include "sound_effect.hpp"
#include "widgets/blank_screen.hpp"

int main(int argc, char** argv) {
    // TODO : Make the playfield not appear when there's no chart selected
    // TODO : Make the linear preview display the end of the chart
    // TODO : Make the linear preview timebar height movable

    auto assets_folder = std::filesystem::path {whereami::executable_dir()} / "assets";

    sf::RenderWindow window(sf::VideoMode(800, 600), "FEIS");
    // sf::RenderWindow& ref_window = window;
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window, false);

    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->Clear();
    IO.Fonts->AddFontFromFileTTF((assets_folder / "fonts" / "NotoSans-Medium.ttf").c_str(), 16.f);
    ImGui::SFML::UpdateFontTexture();

    SoundEffect beatTick {assets_folder / "sounds" / "beat.wav"};
    SoundEffect noteTick {assets_folder / "sounds" / "note.wav"};
    SoundEffect chordTick {assets_folder / "sounds" / "chord.wav"};

    // Loading markers preview
    std::map<std::filesystem::path, sf::Texture> markerPreviews;
    for (const auto& folder : std::filesystem::directory_iterator(assets_folder / "textures" / "markers")) {
        if (folder.is_directory()) {
            sf::Texture markerPreview;
            markerPreview.loadFromFile((folder.path() / "ma15.png").string());
            markerPreview.setSmooth(true);
            markerPreviews.insert({folder, markerPreview});
        }
    }

    Preferences preferences;

    Marker defaultMarker = Marker(preferences.marker);
    Marker& marker = defaultMarker;
    MarkerEndingState markerEndingState = preferences.markerEndingState;

    BlankScreen bg;
    std::optional<EditorState> editorState;
    NotificationsQueue notificationsQueue;
    ESHelper::NewChartDialog newChartDialog;
    ESHelper::ChartPropertiesDialog chartPropertiesDialog;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            switch (event.type) {
                case sf::Event::Closed:
                    preferences.save();
                    if (ESHelper::saveOrCancel(editorState)) {
                        window.close();
                    }
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                    break;
                case sf::Event::MouseButtonPressed:
                    switch (event.mouseButton.button) {
                        case sf::Mouse::Button::Right:
                            if (editorState and editorState->chart) {
                                editorState->chart->creatingLongNote = true;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::MouseButtonReleased:
                    switch (event.mouseButton.button) {
                        case sf::Mouse::Button::Right:
                            if (editorState and editorState->chart) {
                                if (editorState->chart->longNoteBeingCreated) {
                                    auto pair = *editorState->chart->longNoteBeingCreated;
                                    Note new_note = Note(pair.first, pair.second);
                                    std::set<Note> new_note_set = {new_note};
                                    editorState->chart->ref.Notes.insert(new_note);
                                    editorState->chart->longNoteBeingCreated.reset();
                                    editorState->chart->creatingLongNote = false;
                                    editorState->chart->history.push(std::make_shared<ToggledNotes>(new_note_set, true));
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
                            auto delta = static_cast<int>(std::floor(event.mouseWheelScroll.delta));
                            if (delta >= 0) {
                                for (int i = 0; i < delta; ++i) {
                                    Move::backwardsInTime(editorState);
                                }
                            } else {
                                for (int i = 0; i < -delta; ++i) {
                                    Move::forwardsInTime(editorState);
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

                        // Discard, in that order : timeSelection,
                        // selected_notes
                        case sf::Keyboard::Escape:
                            if (editorState and editorState->chart) {
                                if (not std::holds_alternative<std::monostate>(editorState->chart->timeSelection)) {
                                    editorState->chart->timeSelection.emplace<std::monostate>();
                                } else if (not editorState->chart->selectedNotes.empty()) {
                                    editorState->chart->selectedNotes.clear();
                                }
                            }
                            break;

                        // Modify timeSelection
                        case sf::Keyboard::Tab:
                            if (editorState and editorState->chart) {
                                // if no timeSelection was previously made
                                if (std::holds_alternative<std::monostate>(editorState->chart->timeSelection)) {
                                    // set the start of the timeSelection to the
                                    // current time
                                    editorState->chart->timeSelection = static_cast<unsigned int>(editorState->getCurrentTick());

                                    // if the start of the timeSelection is
                                    // already set
                                } else if (std::holds_alternative<unsigned int>(editorState->chart->timeSelection)) {
                                    auto current_tick = static_cast<int>(editorState->getCurrentTick());
                                    auto selection_start =
                                        static_cast<int>(std::get<unsigned int>(editorState->chart->timeSelection));

                                    // if we are on the same tick as the
                                    // timeSelection start we discard the
                                    // timeSelection
                                    if (current_tick == selection_start) {
                                        editorState->chart->timeSelection.emplace<std::monostate>();

                                        // else we create a full timeSelection
                                        // while paying attention to the order
                                    } else {
                                        auto new_selection_start =
                                            static_cast<unsigned int>(std::min(current_tick, selection_start));
                                        auto duration = static_cast<unsigned int>(std::abs(current_tick - selection_start));
                                        editorState->chart->timeSelection.emplace<TimeSelection>(new_selection_start, duration);
                                        editorState->chart->selectedNotes =
                                            editorState->chart->ref.getNotesBetween(new_selection_start, new_selection_start + duration);
                                    }

                                    // if a full timeSelection already exists
                                } else if (std::holds_alternative<TimeSelection>(editorState->chart->timeSelection)) {
                                    // discard the current timeSelection and set
                                    // the start of the timeSelection to the
                                    // current time
                                    editorState->chart->timeSelection = static_cast<unsigned int>(editorState->getCurrentTick());
                                }
                            }
                            break;

                        // Delete selected notes from the chart and discard
                        // timeSelection
                        case sf::Keyboard::Delete:
                            Edit::delete_(editorState, notificationsQueue);
                            break;

                        /*
                         * Arrow keys
                         */
                        case sf::Keyboard::Up:
                            if (event.key.shift) {
                                if (editorState) {
                                    editorState->musicVolumeUp();
                                    std::stringstream ss;
                                    ss << "Music Volume : " << editorState->musicVolume * 10 << "%";
                                    notificationsQueue.push(std::make_shared<TextNotification>(ss.str()));
                                }
                            } else {
                                Move::backwardsInTime(editorState);
                            }
                            break;
                        case sf::Keyboard::Down:
                            if (event.key.shift) {
                                if (editorState) {
                                    editorState->musicVolumeDown();
                                    std::stringstream ss;
                                    ss << "Music Volume : " << editorState->musicVolume * 10 << "%";
                                    notificationsQueue.push(std::make_shared<TextNotification>(ss.str()));
                                }
                            } else {
                                Move::forwardsInTime(editorState);
                            }
                            break;
                        case sf::Keyboard::Left:
                            if (event.key.shift) {
                                if (editorState) {
                                    editorState->musicSpeedDown();
                                    std::stringstream ss;
                                    ss << "Speed : " << editorState->musicSpeed * 10 << "%";
                                    notificationsQueue.push(std::make_shared<TextNotification>(ss.str()));
                                }
                            } else {
                                if (editorState and editorState->chart) {
                                    editorState->snap =
                                        Toolbox::getPreviousDivisor(editorState->chart->ref.getResolution(), editorState->snap);
                                    std::stringstream ss;
                                    ss << "Snap : " << Toolbox::toOrdinal(4 * editorState->snap);
                                    notificationsQueue.push(std::make_shared<TextNotification>(ss.str()));
                                }
                            }
                            break;
                        case sf::Keyboard::Right:
                            if (event.key.shift) {
                                editorState->musicSpeedUp();
                                std::stringstream ss;
                                ss << "Speed : " << editorState->musicSpeed * 10 << "%";
                                notificationsQueue.push(std::make_shared<TextNotification>(ss.str()));
                            } else {
                                if (editorState and editorState->chart) {
                                    editorState->snap =
                                        Toolbox::getNextDivisor(editorState->chart->ref.getResolution(), editorState->snap);
                                    std::stringstream ss;
                                    ss << "Snap : " << Toolbox::toOrdinal(4 * editorState->snap);
                                    notificationsQueue.push(std::make_shared<TextNotification>(ss.str()));
                                }
                            }
                            break;

                        /*
                         * F keys
                         */
                        case sf::Keyboard::F3:
                            if (beatTick.toggle()) {
                                notificationsQueue.push(std::make_shared<TextNotification>("Beat tick : on"));
                            } else {
                                notificationsQueue.push(std::make_shared<TextNotification>("Beat tick : off"));
                            }
                            break;
                        case sf::Keyboard::F4:
                            if (event.key.shift) {
                                if (chordTick.toggle()) {
                                    noteTick.shouldPlay = true;
                                    notificationsQueue.push(std::make_shared<TextNotification>("Note+Chord tick : on"));
                                } else {
                                    noteTick.shouldPlay = false;
                                    notificationsQueue.push(
                                        std::make_shared<TextNotification>("Note+Chord tick : off"));
                                }
                            } else {
                                if (noteTick.toggle()) {
                                    notificationsQueue.push(std::make_shared<TextNotification>("Note tick : on"));
                                } else {
                                    notificationsQueue.push(std::make_shared<TextNotification>("Note tick : off"));
                                }
                            }
                            break;
                        case sf::Keyboard::Space:
                            if (not ImGui::GetIO().WantTextInput) {
                                if (editorState) {
                                    editorState->playing = not editorState->playing;
                                }
                            }
                            break;
                        case sf::Keyboard::Add:
                            if (editorState) {
                                editorState->linearView.zoom_in();
                                notificationsQueue.push(std::make_shared<TextNotification>("Zoom in"));
                            }
                            break;
                        case sf::Keyboard::Subtract:
                            if (editorState) {
                                editorState->linearView.zoom_out();
                                notificationsQueue.push(std::make_shared<TextNotification>("Zoom out"));
                            }
                            break;
                        /*
                         * Letter keys, in alphabetical order
                         */
                        case sf::Keyboard::C:
                            if (event.key.control) {
                                Edit::copy(editorState, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::O:
                            if (event.key.control) {
                                if (ESHelper::saveOrCancel(editorState)) {
                                    ESHelper::open(editorState, assets_folder);
                                }
                            }
                            break;
                        case sf::Keyboard::P:
                            if (event.key.shift) {
                                editorState->showProperties = true;
                            }
                            break;
                        case sf::Keyboard::S:
                            if (event.key.control) {
                                ESHelper::save(*editorState);
                                notificationsQueue.push(std::make_shared<TextNotification>("Saved file"));
                            }
                            break;
                        case sf::Keyboard::V:
                            if (event.key.control) {
                                Edit::paste(editorState, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::X:
                            if (event.key.control) {
                                Edit::cut(editorState, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::Y:
                            if (event.key.control) {
                                Edit::redo(editorState, notificationsQueue);
                            }
                            break;
                        case sf::Keyboard::Z:
                            if (event.key.control) {
                                Edit::undo(editorState, notificationsQueue);
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
        if (editorState) {
            editorState->updateVisibleNotes();
            if (editorState->playing) {
                editorState->previousPos = editorState->playbackPosition;
                editorState->playbackPosition += delta * (editorState->musicSpeed / 10.f);
                if (editorState->music) {
                    switch (editorState->music->getStatus()) {
                        case sf::Music::Stopped:
                        case sf::Music::Paused:
                            if (editorState->playbackPosition.asSeconds() >= 0
                                and editorState->playbackPosition < editorState->music->getDuration()) {
                                editorState->music->setPlayingOffset(editorState->playbackPosition);
                                editorState->music->play();
                            }
                            break;
                        case sf::Music::Playing:
                            editorState->playbackPosition = editorState->music->getPlayingOffset();
                            break;
                        default:
                            break;
                    }
                }
                if (beatTick.shouldPlay) {
                    auto previous_tick = static_cast<int>(editorState->getTicksAt(editorState->previousPos.asSeconds()));
                    auto current_tick = static_cast<int>(editorState->getTicksAt(editorState->playbackPosition.asSeconds()));
                    if (previous_tick / editorState->getResolution() != current_tick / editorState->getResolution()) {
                        beatTick.play();
                    }
                }
                if (noteTick.shouldPlay) {
                    int note_count = 0;
                    for (auto note : editorState->visibleNotes) {
                        float noteTiming = editorState->getSecondsAt(note.getTiming());
                        if (noteTiming >= editorState->previousPos.asSeconds()
                            and noteTiming <= editorState->playbackPosition.asSeconds()) {
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

                if (editorState->playbackPosition > editorState->getPreviewEnd()) {
                    editorState->playing = false;
                    editorState->playbackPosition = editorState->getPreviewEnd();
                }
            } else {
                if (editorState->music) {
                    if (editorState->music->getStatus() == sf::Music::Playing) {
                        editorState->music->pause();
                    }
                }
            }
        }

        // Drawing
        if (editorState) {
            window.clear(sf::Color(0, 0, 0));

            if (editorState->showHistory) {
                editorState->chart->history.display(get_message);
            }
            if (editorState->showPlayfield) {
                editorState->displayPlayfield(marker, markerEndingState);
            }
            if (editorState->showLinearView) {
                editorState->displayLinearView();
            }
            if (editorState->linearView.shouldDisplaySettings) {
                editorState->linearView.displaySettings();
            }
            if (editorState->showProperties) {
                editorState->displayProperties();
            }
            if (editorState->showStatus) {
                editorState->displayStatus();
            }
            if (editorState->showPlaybackStatus) {
                editorState->displayPlaybackStatus();
            }
            if (editorState->showTimeline) {
                editorState->displayTimeline();
            }
            if (editorState->showChartList) {
                editorState->displayChartList();
            }
            if (editorState->showNewChartDialog) {
                std::optional<Chart> c = newChartDialog.display(*editorState);
                if (c) {
                    editorState->showNewChartDialog = false;
                    if (editorState->fumen.Charts.try_emplace(c->dif_name, *c).second) {
                        editorState->chart.emplace(editorState->fumen.Charts.at(c->dif_name));
                    }
                }
            } else {
                newChartDialog.resetValues();
            }
            if (editorState->showChartProperties) {
                chartPropertiesDialog.display(*editorState);
            } else {
                chartPropertiesDialog.shouldRefreshValues = true;
            }

            if (editorState->showSoundSettings) {
                ImGui::Begin("Sound Settings", &editorState->showSoundSettings, ImGuiWindowFlags_AlwaysAutoResize);
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
                    if (ESHelper::saveOrCancel(editorState)) {
                        const char* _filepath = tinyfd_saveFileDialog("New File", nullptr, 0, nullptr, nullptr);
                        if (_filepath != nullptr) {
                            std::filesystem::path filepath(_filepath);
                            try {
                                Fumen f(filepath);
                                f.autoSaveAsMemon();
                                editorState.emplace(f, assets_folder);
                                Toolbox::pushNewRecentFile(std::filesystem::canonical(editorState->fumen.path));
                            } catch (const std::exception& e) {
                                tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                            }
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    if (ESHelper::saveOrCancel(editorState)) {
                        ESHelper::open(editorState, assets_folder);
                    }
                }
                if (ImGui::BeginMenu("Recent Files")) {
                    int i = 0;
                    for (const auto& file : Toolbox::getRecentFiles()) {
                        ImGui::PushID(i);
                        if (ImGui::MenuItem(file.c_str())) {
                            if (ESHelper::saveOrCancel(editorState)) {
                                ESHelper::openFromFile(editorState, file, assets_folder);
                            }
                        }
                        ImGui::PopID();
                        ++i;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Close", "", false, editorState.has_value())) {
                    if (ESHelper::saveOrCancel(editorState)) {
                        editorState.reset();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S", false, editorState.has_value())) {
                    ESHelper::save(*editorState);
                }
                if (ImGui::MenuItem("Save As", "", false, editorState.has_value())) {
                    char const* options[1] = {"*.memon"};
                    const char* _filepath(tinyfd_saveFileDialog("Save File", nullptr, 1, options, nullptr));
                    if (_filepath != nullptr) {
                        std::filesystem::path filepath(_filepath);
                        try {
                            editorState->fumen.saveAsMemon(filepath);
                        } catch (const std::exception& e) {
                            tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Properties", "Shift+P", false, editorState.has_value())) {
                    editorState->showProperties = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    Edit::undo(editorState, notificationsQueue);
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    Edit::redo(editorState, notificationsQueue);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                    Edit::cut(editorState, notificationsQueue);
                }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    Edit::copy(editorState, notificationsQueue);
                }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    Edit::paste(editorState, notificationsQueue);
                }
                if (ImGui::MenuItem("Delete", "Delete")) {
                    Edit::delete_(editorState, notificationsQueue);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Chart", editorState.has_value())) {
                if (ImGui::MenuItem("Chart List")) {
                    editorState->showChartList = true;
                }
                if (ImGui::MenuItem("Properties##Chart", nullptr, false, editorState->chart.has_value())) {
                    editorState->showChartProperties = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("New Chart")) {
                    editorState->showNewChartDialog = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete Chart", nullptr, false, editorState->chart.has_value())) {
                    editorState->fumen.Charts.erase(editorState->chart->ref.dif_name);
                    editorState->chart.reset();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View", editorState.has_value())) {
                if (ImGui::MenuItem("Playfield", nullptr, editorState->showPlayfield)) {
                    editorState->showPlayfield = not editorState->showPlayfield;
                }
                if (ImGui::MenuItem("Linear View", nullptr, editorState->showLinearView)) {
                    editorState->showLinearView = not editorState->showLinearView;
                }
                if (ImGui::MenuItem("Playback Status", nullptr, editorState->showPlaybackStatus)) {
                    editorState->showPlaybackStatus = not editorState->showPlaybackStatus;
                }
                if (ImGui::MenuItem("Timeline", nullptr, editorState->showTimeline)) {
                    editorState->showTimeline = not editorState->showTimeline;
                }
                if (ImGui::MenuItem("Editor Status", nullptr, editorState->showStatus)) {
                    editorState->showStatus = not editorState->showStatus;
                }
                if (ImGui::MenuItem("History", nullptr, editorState->showHistory)) {
                    editorState->showHistory = not editorState->showHistory;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings", editorState.has_value())) {
                if (ImGui::MenuItem("Sound")) {
                    editorState->showSoundSettings = true;
                }
                if (ImGui::MenuItem("Linear View")) {
                    editorState->linearView.shouldDisplaySettings = true;
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
                                tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
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