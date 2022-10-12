#include "history_item.hpp"

#include <functional>
#include <sstream>
#include <tuple>

#include <fmt/core.h>

#include "better_song.hpp"
#include "editor_state.hpp"


const std::string& HistoryItem::get_message() const {
    return message;
}

AddNotes::AddNotes(const std::string& chart, const better::Notes& notes) :
    difficulty_name(chart),
    notes(notes)
{
    if (notes.empty()) {
        throw std::invalid_argument(
            "Can't construct a AddedNotes History Action with an empty note "
            "set"
        );
    }
    message = fmt::format(
        "Added {} note{} to chart {}",
        notes.size(),
        notes.size() > 1 ? "s" : "",
        chart
    );
}

void AddNotes::do_action(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->second.get_time()));
    if (ed.chart_state) {
        if (not (ed.chart_state->difficulty_name == difficulty_name)) {
            ed.open_chart(difficulty_name);
        }
        for (const auto& [_, note] : notes) {
            ed.chart_state->chart.notes.insert(note);
        }
    }
}

void AddNotes::undo_action(EditorState& ed) const {
    ed.set_playback_position(ed.time_at(notes.begin()->second.get_time()));
    if (ed.chart_state) {
        if (not (ed.chart_state->difficulty_name == difficulty_name)) {
            ed.open_chart(difficulty_name);
        }
        for (const auto& [_, note] : notes) {
            ed.chart_state->chart.notes.erase(note);
        }
    }
}


RemoveNotes::RemoveNotes(const std::string& chart, const better::Notes& notes) :
    AddNotes(chart, notes)
{
    if (notes.empty()) {
        throw std::invalid_argument(
            "Can't construct a RemovedNotes History Action with an empty note "
            "set"
        );
    }
    message = fmt::format(
        "Removed {} note{} from chart {}",
        notes.size(),
        notes.size() > 1 ? "s" : "",
        chart
    );
}

void RemoveNotes::do_action(EditorState& ed) const {
    AddNotes::undo_action(ed);
}

void RemoveNotes::undo_action(EditorState& ed) const {
    AddNotes::do_action(ed);
}

RerateChart::RerateChart(
    const std::string& chart,
    const std::optional<Decimal>& old_level,
    const std::optional<Decimal>& new_level
) :
    chart(chart),
    old_level(old_level),
    new_level(new_level)
{
    message = fmt::format(
        "Rerated {} : {} -> {}",
        chart,
        better::stringify_level(old_level),
        better::stringify_level(new_level)
    );
}

void RerateChart::do_action(EditorState& ed) const {
    auto modified_chart = ed.song.charts.extract(chart);
    modified_chart.mapped().level = new_level;
    const auto [_1, inserted, _2] = ed.song.charts.insert(std::move(modified_chart));
    if (not inserted) {
        throw std::runtime_error(
            "Redoing the change of chart level failed : "
            "inserting the modified chart in the song object failed"
        );
    }
}

void RerateChart::undo_action(EditorState& ed) const {
    auto modified_chart = ed.song.charts.extract(chart);
    modified_chart.mapped().level = old_level;
    const auto [_1, inserted, _2] = ed.song.charts.insert(std::move(modified_chart));
    if (not inserted) {
        throw std::runtime_error(
            "Undoing the change of chart level failed : "
            "inserting the modified chart in the song object failed"
        );
    }
}

RenameChart::RenameChart(
    const std::string& old_name,
    const std::string& new_name
) :
    old_name(old_name),
    new_name(new_name)
{
    message = fmt::format(
        "Rename {} -> {}",
        old_name,
        new_name
    );
}

void RenameChart::do_action(EditorState& ed) const {
    auto modified_chart = ed.song.charts.extract(old_name);
    modified_chart.key() = new_name;
    const auto [_1, inserted, _2] = ed.song.charts.insert(std::move(modified_chart));
    if (not inserted) {
        throw std::runtime_error(
            "Redoing the renaming of the chart failed : "
            "inserting the modified chart in the song object failed"
        );
    }
}

void RenameChart::undo_action(EditorState& ed) const {
    auto modified_chart = ed.song.charts.extract(new_name);
    modified_chart.key() = old_name;
    const auto [_1, inserted, _2] = ed.song.charts.insert(std::move(modified_chart));
    if (not inserted) {
        throw std::runtime_error(
            "Undoing the renaming of the chart failed : "
            "inserting the modified chart in the song object failed"
        );
    }
}
