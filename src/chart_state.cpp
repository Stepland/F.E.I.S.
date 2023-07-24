#include "chart_state.hpp"

#include <memory>

#include <fmt/core.h>
#include <SFML/System/Time.hpp>

#include "better_note.hpp"
#include "better_notes.hpp"
#include "generic_interval.hpp"
#include "history_item.hpp"
#include "special_numeric_types.hpp"

ChartState::ChartState(
    better::Chart& c,
    const std::string& name,
    History& history,
    std::filesystem::path assets,
    const config::Config& config_
) :
    chart(c),
    difficulty_name(name),
    config(config_),
    history(history),
    density_graph(assets, config_)
{}

void ChartState::cut(
    NotificationsQueue& nq,
    better::Timing& timing,
    const TimingOrigin& timing_origin
) {
    if (not selected_stuff.empty()) {
        if (not selected_stuff.notes.empty()) {
            const auto message = fmt::format(
                "Cut {} note{}",
                selected_stuff.notes.size(),
                selected_stuff.notes.size() > 1 ? "s" : ""
            );
            nq.push(std::make_shared<TextNotification>(message));
            for (const auto& [_, note] : selected_stuff.notes) {
                chart.notes->erase(note);
            }
            history.push(std::make_shared<RemoveNotes>(difficulty_name, selected_stuff.notes));
            density_graph.should_recompute = true;
        }
        if (not selected_stuff.bpm_events.empty()) {
            const auto before = timing;
            for (const auto& bpm_event : selected_stuff.bpm_events) {
                timing.erase(bpm_event);
            }
            if (before != timing) {
                const auto message = fmt::format(
                    "Cut {} BPM event{}",
                    selected_stuff.bpm_events.size(),
                    selected_stuff.bpm_events.size() > 1 ? "s" : ""
                );
                nq.push(std::make_shared<TextNotification>(message));
                history.push(std::make_shared<ChangeTiming>(before, timing, timing_origin));
            }
        }
    }

    clipboard.copy(selected_stuff);
    selected_stuff.clear();
};

void ChartState::copy(NotificationsQueue& nq) {
    if (not selected_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Copied {} note{}",
            selected_stuff.notes.size(),
            selected_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
    }

    if (not selected_stuff.bpm_events.empty()) {
        const auto message = fmt::format(
            "Copied {} BPM event{}",
            selected_stuff.bpm_events.size(),
            selected_stuff.bpm_events.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
    }

    clipboard.copy(selected_stuff);
    selected_stuff.clear();
};

void ChartState::paste(
    Fraction at_beat,
    NotificationsQueue& nq,
    better::Timing& timing,
    const TimingOrigin& timing_origin
) {
    if (clipboard.empty()) {
        return;
    }

    const auto pasted_stuff = clipboard.paste(at_beat);
    if (not pasted_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Pasted {} note{}",
            pasted_stuff.notes.size(),
            pasted_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        better::Notes overwritten;
        for (const auto& [_, note] : pasted_stuff.notes) {
            auto&& erased = chart.notes->overwriting_insert(note);
            overwritten.merge(std::move(erased));
        }
        if (not overwritten.empty()) {
            history.push(std::make_shared<RemoveNotes>(difficulty_name, overwritten));
        }
        selected_stuff.notes = pasted_stuff.notes;
        history.push(std::make_shared<AddNotes>(difficulty_name, selected_stuff.notes));
        density_graph.should_recompute = true;
    }
    if (not pasted_stuff.bpm_events.empty()) {
        const auto before = timing;
        for (const auto& bpm_event : pasted_stuff.bpm_events) {
            timing.insert(bpm_event);
        }
        if (before != timing) {
            const auto message = fmt::format(
                "Pasted {} BPM event{}",
                selected_stuff.bpm_events.size(),
                selected_stuff.bpm_events.size() > 1 ? "s" : ""
            );
            nq.push(std::make_shared<TextNotification>(message));
            history.push(std::make_shared<ChangeTiming>(before, timing, timing_origin));
        }
    }
};

void ChartState::delete_(
    NotificationsQueue& nq,
    better::Timing& timing,
    const TimingOrigin& timing_origin
) {
    if (not selected_stuff.notes.empty()) {
        history.push(std::make_shared<RemoveNotes>(difficulty_name, selected_stuff.notes));
        nq.push(
            std::make_shared<TextNotification>("Deleted selected notes")
        );
        for (const auto& [_, note] : selected_stuff.notes) {
            chart.notes->erase(note);
        }
        selected_stuff.notes.clear();
        density_graph.should_recompute = true;
    }
    if (not selected_stuff.bpm_events.empty()) {
        const auto before = timing;
        for (const auto& bpm_event : selected_stuff.bpm_events) {
            timing.erase(bpm_event);
        }
        if (before != timing) {
            history.push(std::make_shared<ChangeTiming>(before, timing, timing_origin));
            nq.push(
                std::make_shared<TextNotification>("Deleted selected BPM events")
            );
        }
    }
}

void ChartState::transform_selected_notes(
    std::function<better::Note(const better::Note&)> transform
) {
    if (not selected_stuff.notes.empty()) {
        better::Notes removed = selected_stuff.notes;
        // Erase all the original notes
        for (const auto& [_, note] : selected_stuff.notes) {
            chart.notes->erase(note);
        }
        // create the set of transformed notes independently first :
        // if two transformed notes overwrite each other, better not keep them
        // separate
        better::Notes transformed;
        for (const auto& [_, note] : selected_stuff.notes) {
            const auto transformed_note = transform(note);
            transformed.insert(transformed_note);
        }

        // overwriting insert of the whole set of transformed notes
        for (const auto& [_, transformed_note] : transformed) {
            auto&& erased = chart.notes->overwriting_insert(transformed_note);
            removed.merge(std::move(erased));
        }
        selected_stuff.notes = transformed;
        history.push(std::make_shared<RemoveThenAddNotes>(difficulty_name, removed, transformed));
        density_graph.should_recompute = true;
    }
}

void ChartState::mirror_selection_horizontally(NotificationsQueue& nq) {
    if (not selected_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Mirrored {} note{}",
            selected_stuff.notes.size(),
            selected_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        transform_selected_notes([](const better::Note& n){return n.mirror_horizontally();});
    }
}

void ChartState::mirror_selection_vertically(NotificationsQueue& nq) {
    if (not selected_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Mirrored {} note{}",
            selected_stuff.notes.size(),
            selected_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        transform_selected_notes([](const better::Note& n){return n.mirror_vertically();});
    }
}

void ChartState::rotate_selection_90_clockwise(NotificationsQueue& nq) {
    if (not selected_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Rotated {} note{}",
            selected_stuff.notes.size(),
            selected_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        transform_selected_notes([](const better::Note& n){return n.rotate_90_clockwise();});
    }
}

void ChartState::rotate_selection_90_counter_clockwise(NotificationsQueue& nq) {
    if (not selected_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Rotated {} note{}",
            selected_stuff.notes.size(),
            selected_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        transform_selected_notes([](const better::Note& n){return n.rotate_90_counter_clockwise();});
    }
}

void ChartState::rotate_selection_180(NotificationsQueue& nq) {
    if (not selected_stuff.notes.empty()) {
        const auto message = fmt::format(
            "Rotated {} note{}",
            selected_stuff.notes.size(),
            selected_stuff.notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        transform_selected_notes([](const better::Note& n){return n.rotate_180();});
    }
}

void ChartState::quantize_selection(unsigned int snap, NotificationsQueue& nq) {
    if (selected_stuff.notes.empty()) {
        return;
    }
    const auto message = fmt::format(
        "Quantized {} note{}",
        selected_stuff.notes.size(),
        selected_stuff.notes.size() > 1 ? "s" : ""
    );
    nq.push(std::make_shared<TextNotification>(message));
    transform_selected_notes([=](const better::Note& n){return n.quantize(snap);});
}


Interval<Fraction> ChartState::visible_beats(const sf::Time& playback_position, const better::Timing& timing) {
    /*
    Approach and burst animations last (at most) 16 frames at 30 fps on
    original jubeat markers, so we look for notes that have ended less than
    16/30 seconds ago or that will start in less than 16/30 seconds
    */
    const auto earliest_visible_time = playback_position - sf::seconds(16.f/30.f);
    const auto latest_visible_time = playback_position + sf::seconds(16.f/30.f);
    
    const auto earliest_visible_beat = timing.beats_at(earliest_visible_time);
    const auto latest_visible_beat = timing.beats_at(latest_visible_time);

    return {earliest_visible_beat, latest_visible_beat};
}

void ChartState::update_visible_notes(const sf::Time& playback_position, const better::Timing& timing) {
    const auto bounds = visible_beats(playback_position, timing);
    visible_notes = chart.notes->between(bounds);
    std::map<Fraction, unsigned int> counts;
    std::for_each(
        visible_notes.cbegin(),
        visible_notes.cend(),
        [&](const auto& it){
            counts[it.second.get_time()] += 1;
        }
    );
    visible_chords.clear();
    std::for_each(
        counts.begin(),
        counts.end(),
        [&](const auto& it){
            if (it.second > 1) {
                visible_chords.insert(it.first);
            }
        }
    );
    const auto new_bars = Interval{bounds.start - bounds.start % 4, bounds.end - (bounds.end % 4) + 1};
    if (new_bars != visible_bars) {
        note_numbers.clear();
        for (auto bar = new_bars.start; bar < new_bars.end; bar += 4) {
            std::set<Fraction> times_of_notes_in_bar;
            chart.notes->in(bar, bar + 4, [&](const better::Notes::const_iterator& it){
                /* rule out
                - long notes that have started outside the bar
                - notes that happen right at the beginning of the next bar */
                const auto time = it->second.get_time();
                if (time >= bar and time < bar + 4) {
                    times_of_notes_in_bar.insert(time);
                }
            });
            
            unsigned int i = 1;
            auto it = times_of_notes_in_bar.begin();
            for (; it != times_of_notes_in_bar.end(); i++, ++it) {
                note_numbers[*it] = i;
            }
        }
    }
};

void ChartState::update_colliding_notes(
    const better::Timing &timing,
    const sf::Time &collision_zone
) {
    colliding_notes.clear();
    for (const auto& [_, note] : *chart.notes) {
        if (chart.notes->is_colliding(note, timing, collision_zone)) {
            colliding_notes.insert(note);
        }
    }
}

void ChartState::toggle_note(
    const sf::Time& playback_position,
    std::uint64_t snap,
    const better::Position& button,
    const better::Timing& timing
) {
    better::Notes toggled_notes;
    const auto bounds = visible_beats(playback_position, timing);
    chart.notes->in(
        {bounds.start, bounds.end},
        [&](const better::Notes::const_iterator& it){
            if (it->second.get_position() == button) {
                toggled_notes.insert(it->second);
            }
        }
    );
    if (not toggled_notes.empty()) {
        for (const auto& [_, note] : toggled_notes) {
            chart.notes->erase(note);
        }
        history.push(std::make_shared<RemoveNotes>(difficulty_name, toggled_notes));
    } else {
        const auto rounded_beats = round_beats(
            timing.beats_at(playback_position),
            snap
        );
        const auto new_note = better::TapNote{rounded_beats, button};
        toggled_notes.insert(new_note);
        chart.notes->insert(new_note);
        history.push(std::make_shared<AddNotes>(difficulty_name, toggled_notes));
    }
    density_graph.should_recompute = true;
}

void ChartState::handle_time_selection_tab(Fraction beats) {
    if (not time_selection) {
        time_selection.emplace(beats, beats);
    } else {
        if (time_selection->width() == 0) {
            *time_selection += beats;
            selected_stuff.notes = chart.notes->between(*time_selection);
        } else {
            time_selection.emplace(beats, beats);
        }
    }
}

void ChartState::insert_long_note_just_created(std::uint64_t snap) {
    if (not long_note_being_created) {
        return;
    }
    auto new_note = make_long_note_dummy_for_linear_view(
        *long_note_being_created,
        Fraction{1, snap}
    );
    better::Notes new_notes;
    new_notes.insert(new_note);
    const auto& overwritten = chart.notes->overwriting_insert(new_note);
    long_note_being_created.reset();
    creating_long_note = false;
    if (not overwritten.empty()) {
        history.push(std::make_shared<RemoveNotes>(difficulty_name, overwritten));
    }
    history.push(std::make_shared<AddNotes>(difficulty_name,new_notes));
}