#pragma once

#include <filesystem>
#include <optional>
#include <variant>
#include <utility>

#include "better_note.hpp"
#include "better_notes.hpp"
#include "better_song.hpp"
#include "config.hpp"
#include "generic_interval.hpp"
#include "history.hpp"
#include "long_note_dummy.hpp"
#include "clipboard.hpp"
#include "notifications_queue.hpp"
#include "widgets/density_graph.hpp"

struct ChartState {
    ChartState(
        better::Chart& c,
        const std::string& name,
        History& history,
        std::filesystem::path assets,
        const config::Config& config
    );
    better::Chart& chart;
    const std::string& difficulty_name;
    
    const config::Config& config;

    void cut(
        NotificationsQueue& nq,
        better::Timing& timing,
        const TimingOrigin& timing_origin
    );
    void copy(NotificationsQueue& nq);
    void paste(
        Fraction at_beat,
        NotificationsQueue& nq,
        better::Timing& timing,
        const TimingOrigin& timing_origin
    );
    void delete_(
        NotificationsQueue& nq,
        better::Timing& timing,
        const TimingOrigin& timing_origin
    );

    void transform_selected_notes(std::function<better::Note(const better::Note&)> transform);
    void mirror_selected_horizontally(NotificationsQueue& nq);
    void mirror_selected_vertically(NotificationsQueue& nq);
    void rotate_selected_90_clockwise(NotificationsQueue& nq);
    void rotate_selected_90_counter_clockwise(NotificationsQueue& nq);
    void rotate_selected_180(NotificationsQueue& nq);

    Interval<Fraction> visible_beats(const sf::Time& playback_position, const better::Timing& timing);
    void update_visible_notes(const sf::Time& playback_position, const better::Timing& timing);
    better::Notes visible_notes;

    void toggle_note(
        const sf::Time& playback_position,
        std::uint64_t snap,
        const better::Position& button,
        const better::Timing& timing
    );

    NoteAndBPMSelection selected_stuff;
    Clipboard clipboard;

    void handle_time_selection_tab(Fraction beats);
    std::optional<Interval<Fraction>> time_selection;

    /*
    The long note currently being created, represented as a pair of tap notes.
    Time span of the represented long note is the span between both taps
    1st note's position defines the long note's position
    2nd note's position suggests where the tail should start
    */
    std::optional<TapNotePair> long_note_being_created;

    // Is the user currently holding right click ? (over the playfield or not)
    bool creating_long_note;

    void insert_long_note_just_created(std::uint64_t snap);

    History& history;
    DensityGraph density_graph;
};
