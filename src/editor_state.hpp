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
#include "time_interval.hpp"
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
 * file
 */
class EditorState {
public:
    EditorState(
        const better::Song& song,
        const std::filesystem::path& assets,
        const std::filesystem::path& save_path
    );

    better::Song song;
    std::optional<std::filesystem::path> song_path;

    std::optional<ChartState> chart_state;

    std::optional<MusicState> music_state;

    Playfield playfield;
    LinearView linear_view;

    int snap = 1;

    std::optional<sf::Texture> jacket;

    bool playing;

    sf::Time previous_playback_position;
    sf::Time playback_position;

    const TimeInterval& get_editable_range();

    void set_playback_position(sf::Time new_position);

    float current_beats();
    float beats_at(sf::Time time);
    float seconds_at(Fraction beat);
    Fraction get_snap_step();

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
    /*
    sf::Time bounds (in the audio file "coordinates") which are accessible
    (and maybe editable) from the editor, can extend before and after
    the actual audio file
    */
    TimeInterval editable_range;

    void reload_album_cover();
    void reload_editable_range();
    
    std::string music_path_in_gui;
    void reload_music();

    better::Timing& applicable_timing;
    void reload_applicable_timing();

    void open_chart(better::Chart& chart);

    std::filesystem::path assets;
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
