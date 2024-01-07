#pragma once

#include <future>
#include <memory>
#include <optional>

#include <SFML/Audio.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Graphics.hpp>
#include <type_traits>

#include "config.hpp"
#include "custom_sfml_audio/beat_ticks.hpp"
#include "custom_sfml_audio/chord_claps.hpp"
#include "custom_sfml_audio/note_claps.hpp"
#include "custom_sfml_audio/open_music.hpp"
#include "custom_sfml_audio/synced_sound_streams.hpp"
#include "guess_tempo.hpp"
#include "utf8_sfml.hpp"
#include "utf8_sfml_redefinitions.hpp"
#include "waveform.hpp"
#include "widgets/linear_view.hpp"
#include "better_note.hpp"
#include "better_song.hpp"
#include "chart_state.hpp"
#include "generic_interval.hpp"
#include "history.hpp"
#include "marker.hpp"
#include "markers.hpp"
#include "clipboard.hpp"
#include "notifications_queue.hpp"
#include "playfield.hpp"


const std::string music_stream = "music";
const std::string note_clap_stream = "note_clap";
const std::string chord_clap_stream = "chord_clap";
const std::string beat_tick_stream = "beat_tick";

/*
 * The god class, holds everything there is to know about the currently open
 * file
 */
class EditorState {
public:
    EditorState(
        const std::filesystem::path& assets,
        config::Config& config
    );
    EditorState(
        const better::Song& song,
        const std::filesystem::path& assets,
        const std::filesystem::path& save_path,
        config::Config& config
    );

    config::Config& config;
    
    History history;

    better::Song song;

    std::optional<std::filesystem::path> song_path;

    std::optional<ChartState> chart_state;

    SyncedSoundStreams audio;
    std::shared_ptr<NoteClaps> note_claps;
    std::shared_ptr<ChordClaps> chord_claps;
    std::shared_ptr<BeatTicks> beat_ticks;
    std::optional<std::shared_ptr<OpenMusic>> music = {};
    bool is_playing_preview_music_from_sss = false;

    std::future<std::optional<waveform::Waveform>> waveform_loader;
    std::optional<waveform::Waveform> waveform;
    waveform::Status waveform_status();

    std::future<std::vector<TempoCandidate>> tempo_candidates_loader;
    std::optional<std::vector<TempoCandidate>> tempo_candidates;

    int get_volume() const;
    void set_volume(int newMusicVolume);
    void volume_up();
    void volume_down();

    /* These speed dials also work when no music is loaded */
    int get_speed() const;
    void set_speed(int newMusicSpeed);
    void speed_up();
    void speed_down();

    std::optional<feis::Music> preview_audio;

    void play_music_preview();
    void stop_music_preview();
    bool music_preview_is_playing() const;
    sf::Time music_preview_position() const;
    sf::Time music_preview_duration() const;
    void update_music_preview_status();

    Playfield playfield;
    LinearView linear_view;

    std::uint64_t snap = 1;

    std::optional<feis::Texture> jacket;

    bool playing;

    std::variant<sf::Time, Fraction> playback_position;
    std::variant<sf::Time, Fraction> previous_playback_position;

    const Interval<sf::Time>& get_editable_range();

    bool has_any_audio() const;
    void toggle_playback();
    void play_note_claps(bool on);
    void toggle_note_claps();
    void play_clap_on_long_note_ends(bool on);
    void toggle_clap_on_long_note_ends();
    void play_chord_claps(bool on);
    void toggle_chord_claps();
    void play_beat_ticks(bool on);
    void toggle_beat_ticks();
    void play();
    void pause();
    void stop();
    sf::SoundSource::Status get_status();
    void set_pitch(float pitch);
    float get_pitch() const;
    void set_playback_position(std::variant<sf::Time, Fraction> newPosition);
    sf::Time get_precise_playback_position();

    Fraction current_exact_beats() const;
    Fraction current_snaped_beats() const;
    Fraction previous_exact_beats() const;
    sf::Time current_time() const;
    sf::Time previous_time() const;
    Fraction beats_at(sf::Time time) const;
    sf::Time time_at(Fraction beat) const;
    Fraction get_snap_step() const;

    bool& show_playfield;
    void display_playfield(const Markers::marker_type& marker, Judgement markerEndingState);

    bool& show_playfield_settings;
    void display_playfield_settings();

    bool& show_file_properties;
    void display_file_properties();

    bool& show_debug;
    void display_debug();

    bool& show_playback_status;
    void display_playback_status();

    bool& show_timeline;
    void display_timeline();

    bool& show_chart_list;
    void display_chart_list();

    bool& show_linear_view;
    void display_linear_view();

    bool& show_sound_settings;
    void display_sound_settings();

    bool& show_editor_settings;
    void display_editor_settings();

    bool& show_history;
    void display_history();

    bool& show_new_chart_dialog;
    bool& show_chart_properties;
    
    bool& show_sync_menu;
    void display_sync_menu();

    bool& show_bpm_change_menu;
    void display_bpm_change_menu();

    bool& show_timing_kind_menu;
    void display_timing_kind_menu();

    enum class SaveOutcome {
        UserSaved,
        UserDeclindedSaving,
        UserCanceled,
        NoSavingNeeded,
    };

    SaveOutcome save_if_needed_and_user_wants_to();

    SaveOutcome save_if_needed();

    SaveOutcome save_asking_for_path();

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

    void insert_long_note_just_created();

    void move_backwards_in_time();
    void move_forwards_in_time();

    void undo(NotificationsQueue& nq);
    void redo(NotificationsQueue& nq);

    void cut(NotificationsQueue& nq);
    void copy(NotificationsQueue& nq);
    void paste(NotificationsQueue& nq);
    void delete_(NotificationsQueue& nq);

    void discard_selection();

    void save(const std::filesystem::path& path);

    void insert_chart(const std::string& name, const better::Chart& chart);
    void insert_chart_and_push_history(const std::string& name, const better::Chart& chart);
    void erase_chart(const std::string& name);
    void erase_chart_and_push_history(const std::string& name);
    void open_chart(const std::string& name);
    void close_chart();

    void update_visible_notes();

    void reload_jacket();
    void reload_music();
    void reload_preview_audio();
    void reload_applicable_timing();
    void reload_sounds_that_depend_on_notes();
    void reload_sounds_that_depend_on_timing();
    void reload_all_sounds();
    void reload_editable_range();
    void reload_colliding_notes();

    void frame_hook();

    TimingOrigin timing_origin();
    void switch_to_chart_timing();
    void switch_to_chart_timing_and_push_history();
    void discard_chart_timing();
    void discard_chart_timing_and_push_history();
    void overwrite_song_with_chart_timing();
    void overwrite_song_with_chart_timing_and_push_history();

private:

    int speed = 10;  // 1 -> 20

    bool note_clap_stream_is_on() const;
    bool beat_tick_stream_is_on() const;

    void set_volumes_from_config();


    // Playback status used when there is no actual audio being played
    sf::SoundSource::Status status = sf::SoundSource::Stopped;

    /*
    sf::Time bounds (in the audio file "coordinates") which are accessible
    (and maybe editable) from the editor, can extend before and after
    the audio file
    */
    Interval<sf::Time> editable_range;
    Interval<sf::Time> choose_editable_range();
    void clear_music();

    std::shared_ptr<better::Timing> applicable_timing;
    void replace_applicable_timing_with(const better::Timing& new_timing);

    std::filesystem::path assets;

    std::optional<std::filesystem::path> full_audio_path();
};

namespace feis {
    void force_save(
        std::optional<EditorState>& ed,
        NotificationsQueue& nq
    );

    void save_ask_open(
        std::optional<EditorState>& ed,
        const std::filesystem::path& assets,
        const std::filesystem::path& settings,
        config::Config& config
    );

    void save_open(
        std::optional<EditorState>& ed,
        const std::filesystem::path& file,
        const std::filesystem::path& assets,
        const std::filesystem::path& settings,
        config::Config& config
    );

    void open_from_file(
        std::optional<EditorState>& ed,
        const std::filesystem::path& file,
        const std::filesystem::path& assets,
        const std::filesystem::path& settings,
        config::Config& config
    );

    void save_close(std::optional<EditorState>& ed);

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

    void display_shortcuts_help(bool& show);
    void display_about_menu(bool& show);
}
