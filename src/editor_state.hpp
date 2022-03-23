#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <optional>

#include "better_song.hpp"
#include "chart_state.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "marker.hpp"
#include "metadata_in_gui.hpp"
#include "music_state.hpp"
#include "notes_clipboard.hpp"
#include "notifications_queue.hpp"
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

    std::optional<std::filesystem::path> song_path;

    std::optional<ChartState> chart_state;

    std::optional<MusicState> music_state;

    std::optional<sf::Music> preview_audio;

    Playfield playfield;
    LinearView linear_view;

    unsigned int snap = 1;

    std::optional<sf::Texture> jacket;

    bool playing;

    sf::Time playback_position;
    sf::Time previous_playback_position;

    const TimeInterval& get_editable_range();

    void set_playback_position(sf::Time new_position);

    Fraction current_exact_beats() const;
    Fraction current_snaped_beats() const;
    Fraction beats_at(sf::Time time) const;
    sf::Time time_at(Fraction beat) const;
    Fraction get_snap_step() const;

    bool showPlayfield = true;
    void display_playfield(Marker& marker, MarkerEndingState markerEndingState);

    bool showProperties;
    void display_properties();

    bool showStatus;
    void display_status();

    bool showPlaybackStatus = true;
    void display_playback_status();

    bool showTimeline = true;
    void display_timeline();

    bool showChartList;
    void display_chart_list(std::filesystem::path assets);

    bool showLinearView;
    void display_linear_view();

    bool showNewChartDialog;
    bool showChartProperties;
    bool showHistory;
    bool showSoundSettings;

    saveChangesResponses alertSaveChanges();
    bool saveChangesOrCancel();

    void updateVisibleNotes();
    better::Notes visibleNotes;

    void toggleNoteAtCurrentTime(int pos);

    void move_backwards_in_time();
    void move_forwards_in_time();

    void undo(NotificationsQueue& nq);
    void redo(NotificationsQueue& nq);

    void cut(NotificationsQueue& nq);
    void copy(NotificationsQueue& nq);
    void paste(NotificationsQueue& nq);
    void delete_(NotificationsQueue& nq);

private:

    better::Song song;

    MetadataInGui metadata_in_gui;
    /*
    sf::Time bounds (in the audio file "coordinates") which are accessible
    (and maybe editable) from the editor, can extend before and after
    the audio file
    */
    TimeInterval editable_range;
    void reload_editable_range();
    void reload_jacket();
    void reload_music();
    void reload_preview_audio();

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
