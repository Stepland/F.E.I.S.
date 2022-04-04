#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <optional>

#include "better_song.hpp"
#include "chart_state.hpp"
#include "generic_interval.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "marker.hpp"
#include "metadata_in_gui.hpp"
#include "music_state.hpp"
#include "notes_clipboard.hpp"
#include "notifications_queue.hpp"
#include "precise_music.hpp"
#include "src/better_note.hpp"
#include "widgets/linear_view.hpp"
#include "widgets/playfield.hpp"


/*
 * The god class, holds everything there is to know about the currently open
 * file
 */
class EditorState {
public:
    explicit EditorState(const std::filesystem::path& assets);
    EditorState(
        const better::Song& song,
        const std::filesystem::path& assets,
        const std::filesystem::path& save_path
    );
        
    better::Song song;

    std::optional<std::filesystem::path> song_path;

    std::optional<ChartState> chart_state;

    std::optional<MusicState> music_state;

    std::optional<sf::Music> preview_audio;

    Playfield playfield;
    LinearView linear_view;

    std::uint64_t snap = 1;

    std::optional<sf::Texture> jacket;

    bool playing;

    sf::Time playback_position;
    sf::Time previous_playback_position;

    const Interval<sf::Time>& get_editable_range();

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
    void display_chart_list();

    bool showLinearView;
    void display_linear_view();

    bool showNewChartDialog;
    bool showChartProperties;
    bool showHistory;
    bool showSoundSettings;

    enum class SaveOutcome {
        UserSaved,
        UserDeclindedSaving,
        UserCanceled,
        NoSavingNeeded,
    };

    SaveOutcome save_if_needed();

    bool needs_to_save() const;

    enum class UserWantsToSave {
        Yes,
        No,
        Cancel,
    };

    UserWantsToSave ask_if_user_wants_to_save() const;

    /*
    If the given song already has a dedicated file on disk, returns its path.
    Otherwise use a dialog box to ask the user for a path and return it, or
    return nothing if the user canceled
    */
    std::optional<std::filesystem::path> ask_for_save_path_if_needed();

    void toggle_note_at_current_time(const better::Position& pos);

    void move_backwards_in_time();
    void move_forwards_in_time();

    void undo(NotificationsQueue& nq);
    void redo(NotificationsQueue& nq);

    void save(const std::filesystem::path& path);

    void open_chart(const std::string& name);

private:

    /*
    sf::Time bounds (in the audio file "coordinates") which are accessible
    (and maybe editable) from the editor, can extend before and after
    the audio file
    */
    Interval<sf::Time> editable_range;
    void reload_editable_range();
    void reload_jacket();
    void reload_music();
    void reload_preview_audio();

    better::Timing& applicable_timing;
    void reload_applicable_timing();

    std::filesystem::path assets;
};

namespace feis {
    void open(std::optional<EditorState>& ed, std::filesystem::path assets, std::filesystem::path settings);
    void open_from_file(
        std::optional<EditorState>& ed,
        std::filesystem::path file,
        std::filesystem::path assets,
        std::filesystem::path settings
    );

    class NewChartDialog {
    public:
        std::optional<std::pair<std::string, better::Chart>> display(EditorState& editorState);
        void resetValues() {
            level = 1;
            difficulty = "";
            combo_preview = "";
            show_custom_dif_name = false;
        };

    private:
        Decimal level = 1;
        std::string difficulty;
        std::string combo_preview;
        bool show_custom_dif_name = false;
    };

    class ChartPropertiesDialog {
    public:
        void display(EditorState& editorState);
        bool should_refresh_values = true;

    private:
        Decimal level;
        std::string difficulty_name;
        std::string combo_preview;
        std::set<std::string> difficulty_names_in_use;
        bool show_custom_dif_name = false;
    };
}
