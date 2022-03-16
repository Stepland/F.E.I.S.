#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <optional>

#include "better_song.hpp"
#include "chart_state.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "marker.hpp"
#include "music_state.hpp"
#include "notes_clipboard.hpp"
#include "precise_music.hpp"
#include "time_selection.hpp"
#include "widgets/linear_view.hpp"
#include "widgets/playfield.hpp"

class ActionWithMessage;
class OpenChart;

enum saveChangesResponses {
    saveChangesYes,
    saveChangesNo,
    saveChangesCancel,
    saveChangesDidNotDisplayDialog
};

/*
 * The god class, holds everything there is to know about the currently open
 * .memon file
 */
class EditorState {
public:
    EditorState(
        const better::Song& song,
        const std::filesystem::path& song_path,
        const std::filesystem::path& assets
    ) :
        song(song),
        playfield(assets),
        linear_view(assets),
        edited_music_path(song.metadata.audio.value_or("")),
        song_path(song_path)
    {
        if (not this->song.charts.empty()) {
            this->chart_state.emplace(this->song.charts.begin()->second, assets);
        }
        reload_music();
        reload_album_cover();
    };

    better::Song song;
    std::optional<ChartState> chart_state;

    std::optional<MusicState> music_state;

    Playfield playfield;
    LinearView linear_view;

    // the snap but divided by 4 because you can't set a snap to anything lower
    // than 4ths
    int snap = 1;

    std::optional<sf::Texture> album_cover;

    bool playing;

    sf::Time previous_pos;
    sf::Time playback_position;

    const sf::Time& get_preview_end();

    void set_playback_and_music_position(sf::Time new_position);

    float getBeats() { return getBeatsAt(playback_position.asSeconds()); };
    float getBeatsAt(float seconds) {
        return ((seconds + song.offset) / 60.f) * song.BPM;
    };
    float getCurrentTick() { return getTicksAt(playback_position.asSeconds()); };
    float getTicksAt(float seconds) {
        return getBeatsAt(seconds) * get_resolution();
    }
    float getSecondsAt(int tick) {
        return (60.f * tick) / (song.BPM * get_resolution()) - song.offset;
    };
    int get_resolution() { return chart_state ? chart_state->chart.getResolution() : 240; };
    int get_snap_step() { return get_resolution() / snap; };


    void reload_album_cover();
    void reload_preview_end();

    bool showPlayfield = true;
    bool showProperties;
    bool showStatus;
    bool showPlaybackStatus = true;
    bool showTimeline = true;
    bool showChartList;
    bool showNewChartDialog;
    bool showChartProperties;
    bool showHistory;
    bool showSoundSettings;
    bool showLinearView;

    void displayPlayfield(Marker& marker, MarkerEndingState markerEndingState);
    void displayProperties();
    void displayStatus();
    void displayPlaybackStatus();
    void displayTimeline();
    void displayChartList(std::filesystem::path assets);
    void displayLinearView();

    saveChangesResponses alertSaveChanges();
    bool saveChangesOrCancel();

    void updateVisibleNotes();
    std::set<Note> visibleNotes;

    void toggleNoteAtCurrentTime(int pos);

private:
    sf::Time preview_end;  // sf::Time (in the audio file "coordinates") at which the chart preview stops, can be
                        // after the end of the actual audio file
    
    std::string edited_music_path;
    void reload_music();

    std::filesystem::path song_path;
};

namespace ESHelper {
    void save(EditorState& ed);
    void open(std::optional<EditorState>& ed, std::filesystem::path assets, std::filesystem::path settings);
    void openFromFile(
        std::optional<EditorState>& ed,
        std::filesystem::path file,
        std::filesystem::path assets,
        std::filesystem::path settings
    );

    bool saveOrCancel(std::optional<EditorState>& ed);

    class NewChartDialog {
    public:
        std::optional<Chart> display(EditorState& editorState);
        void resetValues() {
            level = 1;
            resolution = 240;
            difficulty = "";
            comboPreview = "";
            showCustomDifName = false;
        };

    private:
        int level = 1;
        int resolution = 240;
        std::string difficulty;
        std::string comboPreview;
        bool showCustomDifName = false;
    };

    class ChartPropertiesDialog {
    public:
        void display(EditorState& editorState, std::filesystem::path assets);
        bool shouldRefreshValues = true;

    private:
        int level;
        std::string difficulty_name;
        std::string comboPreview;
        std::set<std::string> difNamesInUse;
        bool showCustomDifName = false;
    };
}
