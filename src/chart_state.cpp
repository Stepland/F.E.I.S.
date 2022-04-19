#include "chart_state.hpp"

#include <memory>

#include <fmt/core.h>
#include <SFML/System/Time.hpp>

#include "better_note.hpp"
#include "better_notes.hpp"
#include "history_item.hpp"
#include "special_numeric_types.hpp"

ChartState::ChartState(better::Chart& c, const std::string& name, std::filesystem::path assets) :
    chart(c),
    difficulty_name(name),
    density_graph(assets)
{
    history.push(std::make_shared<OpenChart>(c, name));
}

void ChartState::cut(NotificationsQueue& nq) {
    if (not selected_notes.empty()) {
        const auto message = fmt::format(
            "Cut {} note{}",
            selected_notes.size(),
            selected_notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));

        notes_clipboard.copy(selected_notes);
        for (const auto& [_, note] : selected_notes) {
            chart.notes.erase(note);
        }
        history.push(std::make_shared<RemoveNotes>(selected_notes));
        selected_notes.clear();
    }
};

void ChartState::copy(NotificationsQueue& nq) {
    if (not selected_notes.empty()) {
        const auto message = fmt::format(
            "Copied {} note{}",
            selected_notes.size(),
            selected_notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        notes_clipboard.copy(selected_notes);
    }
};

void ChartState::paste(Fraction at_beat, NotificationsQueue& nq) {
    if (not notes_clipboard.empty()) {
        const auto pasted_notes = notes_clipboard.paste(at_beat);
        const auto message = fmt::format(
            "Pasted {} note{}",
            pasted_notes.size(),
            pasted_notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        for (const auto& [_, note] : pasted_notes) {
            chart.notes.overwriting_insert(note);
        }
        selected_notes = pasted_notes;
        history.push(std::make_shared<AddNotes>(selected_notes));
        density_graph.should_recompute = true;
    }
};

void ChartState::delete_(NotificationsQueue& nq) {
    if (selected_notes.empty()) {
        history.push(std::make_shared<RemoveNotes>(selected_notes));
        nq.push(
            std::make_shared<TextNotification>("Deleted selected notes")
        );
        for (const auto& [_, note] : selected_notes) {
            chart.notes.erase(note);
        }
        selected_notes.clear();
    }
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
    visible_notes = chart.notes.between(bounds);
};

void ChartState::toggle_note(
    const sf::Time& playback_position,
    std::uint64_t snap,
    const better::Position& button,
    const better::Timing& timing
) {
    better::Notes toggled_notes;
    const auto bounds = visible_beats(playback_position, timing);
    chart.notes.in(
        {bounds.start, bounds.end},
        [&](const better::Notes::const_iterator& it){
            if (it->second.get_position() == button) {
                toggled_notes.insert(it->second);
            }
        }
    );
    if (not toggled_notes.empty()) {
        for (const auto& [_, note] : toggled_notes) {
            chart.notes.erase(note);
        }
        history.push(std::make_shared<RemoveNotes>(toggled_notes));
    } else {
        const auto rounded_beats = round_beats(
            timing.beats_at(playback_position),
            snap
        );
        const auto new_note = better::TapNote{rounded_beats, button};
        toggled_notes.insert(new_note);
        chart.notes.insert(new_note);
        history.push(std::make_shared<AddNotes>(toggled_notes));
    }
    density_graph.should_recompute = true;
}

void ChartState::handle_time_selection_tab(Fraction beats) {
    if (not time_selection) {
        time_selection.emplace(beats, beats);
    } else {
        if (time_selection->width() == 0) {
            *time_selection += beats;
            selected_notes = chart.notes.between(*time_selection);
        } else {
            time_selection.emplace(beats, beats);
        }
    }
}