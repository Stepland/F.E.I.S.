#include "chart_state.hpp"

#include <memory>

#include <fmt/core.h>
#include <SFML/System/Time.hpp>

#include "better_note.hpp"
#include "better_notes.hpp"
#include "history_actions.hpp"
#include "special_numeric_types.hpp"

ChartState::ChartState(better::Chart& c, const std::string& name, std::filesystem::path assets) :
    chart(c),
    difficulty_name(name),
    density_graph(assets)
{
    history.push(std::make_shared<OpenChart>(c));
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

void ChartState::paste(NotificationsQueue& nq, Fraction at_beat) {
    if (not notes_clipboard.empty()) {
        const auto pasted_notes = notes_clipboard.paste(at_beat);
        const auto message = fmt::format(
            "Pasted {} note{}",
            pasted_notes.size(),
            pasted_notes.size() > 1 ? "s" : ""
        );
        nq.push(std::make_shared<TextNotification>(message));
        for (const auto& note : pasted_notes) {
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

Interval<Fraction> ChartState::visible_beats(const sf::Time& playback_position) {
    /*
    Approach and burst animations last (at most) 16 frames at 30 fps on
    original jubeat markers, so we look for notes that have ended less than
    16/30 seconds ago or that will start in less than 16/30 seconds
    */
    const auto earliest_visible_time = playback_position - sf::seconds(16.f/30.f);
    const auto latest_visible_time = playback_position + sf::seconds(16.f/30.f);
    
    const auto earliest_visible_beat = chart.timing.beats_at(earliest_visible_time);
    const auto latest_visible_beat = chart.timing.beats_at(latest_visible_time);

    return {earliest_visible_beat, latest_visible_beat};
}

void ChartState::update_visible_notes(const sf::Time& playback_position) {
    visible_notes.clear();
    const auto bounds = visible_beats(playback_position);
    chart.notes.in(
        {bounds.start, bounds.end},
        [this](const better::Notes::const_iterator& it){
            this->visible_notes.push_back(it->second);
        }
    );
};

void ChartState::toggle_note(
    const sf::Time& playback_position,
    std::uint64_t snap,
    const better::Position& button
) {
    std::vector<better::Note> toggled_notes = {};
    const auto bounds = visible_beats(playback_position);
    chart.notes.in(
        {bounds.start, bounds.end},
        [&](const better::Notes::const_iterator& it){
            if (it->second.get_position() == button) {
                toggled_notes.push_back(it->second);
            }
        }
    );
    if (not toggled_notes.empty()) {
        for (const auto& note : toggled_notes) {
            chart.notes.erase(note);
        }
        history.push(std::make_shared<RemoveNotes>(toggled_notes));
    } else {
        const auto rounded_beats = round_beats(
            chart.timing.beats_at(playback_position),
            snap
        );
        const auto new_note = better::TapNote{rounded_beats, button};
        toggled_notes.push_back(new_note);
        chart.notes.insert(new_note);
        history.push(std::make_shared<AddNotes>(toggled_notes));
    }
    density_graph.should_recompute = true;
}

better::LongNote make_long_note_dummy(
    Fraction current_beat,
    const TapNotePair& long_note_being_created
) {
    const auto note = make_long_note(long_note_being_created);
    return better::LongNote{
        current_beat,
        note.get_position(),
        note.get_duration(),
        note.get_tail_tip()
    };
};

better::LongNote make_long_note(
    Fraction current_beat,
    const TapNotePair& long_note_being_created
) {
    auto start_time = long_note_being_created.first.get_time();
    auto end_time = long_note_being_created.second.get_time();
    if (start_time > end_time) {
        std::swap(start_time, end_time);
    }
    const auto duration = boost::multiprecision::abs(start_time - end_time);
    return better::LongNote(
        start_time,
        long_note_being_created.first.get_position(),
        duration,
        closest_tail_position(
            long_note_being_created.first.get_position(),
            long_note_being_created.second.get_position()
        )
    );
};

better::Position closest_tail_position(
    const better::Position& anchor,
    const better::Position& requested_tail
) {
    if (anchor == requested_tail) {
        return smallest_possible_tail(anchor);
    }

    const auto delta_x = static_cast<int>(requested_tail.get_x()) - static_cast<int>(anchor.get_x());
    const auto delta_y = static_cast<int>(requested_tail.get_y()) - static_cast<int>(anchor.get_y());
    if (std::abs(delta_x) > std::abs(delta_y)) {
        return better::Position(anchor.get_x(), requested_tail.get_y());
    } else {
        return better::Position(requested_tail.get_x(), anchor.get_y());
    }
};

better::Position smallest_possible_tail(const better::Position& anchor) {
    if (anchor.get_x() > 0) {
        return better::Position{anchor.get_x() - 1, anchor.get_y()};
    } else {
        return better::Position{anchor.get_x() + 1, anchor.get_y()};
    }
};
