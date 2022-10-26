#include "history_item.hpp"

#include <functional>
#include <sstream>
#include <tuple>

#include <fmt/core.h>
#include <variant>

#include "better_song.hpp"
#include "editor_state.hpp"
#include "src/better_metadata.hpp"
#include "src/better_timing.hpp"


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
            ed.chart_state->chart.notes->insert(note);
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
            ed.chart_state->chart.notes->erase(note);
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

ChangeTitle::ChangeTitle(
    const std::string& old_value,
    const std::string& new_value
) :
    ChangeValue(old_value, new_value)
{
    message = fmt::format(
        "Change song title : \"{}\" -> \"{}\"",
        old_value,
        new_value
    );
}

void ChangeTitle::set_value(EditorState& ed, const std::string& value) const {
    ed.song.metadata.title = value;
}

ChangeArtist::ChangeArtist(
    const std::string& old_value,
    const std::string& new_value
) :
    ChangeValue(old_value, new_value)
{
    message = fmt::format(
        "Change song artist : \"{}\" -> \"{}\"",
        old_value,
        new_value
    );
}

void ChangeArtist::set_value(EditorState& ed, const std::string& value) const {
    ed.song.metadata.artist = value;
}

ChangeAudio::ChangeAudio(
    const std::string& old_value,
    const std::string& new_value
) :
    ChangeValue(old_value, new_value)
{
    message = fmt::format(
        "Change audio : \"{}\" -> \"{}\"",
        old_value,
        new_value
    );
}

void ChangeAudio::set_value(EditorState& ed, const std::string& value) const {
    ed.song.metadata.audio = value;
    ed.reload_music();
}

ChangeJacket::ChangeJacket(
    const std::string& old_value,
    const std::string& new_value
) :
    ChangeValue(old_value, new_value)
{
    message = fmt::format(
        "Change jacket : \"{}\" -> \"{}\"",
        old_value,
        new_value
    );
}

void ChangeJacket::set_value(EditorState& ed, const std::string& value) const {
    ed.song.metadata.jacket = value;
    ed.reload_jacket();
}

ChangePreview::ChangePreview(
    const PreviewState& old_value,
    const PreviewState& new_value
) :
    ChangeValue(old_value, new_value)
{
    message = fmt::format(
        "Change preview : {} -> {}",
        old_value,
        new_value
    );
}

void ChangePreview::set_value(EditorState& ed, const PreviewState& value) const {
    const auto set_value_ = VariantVisitor {
        [&](const better::PreviewLoop& loop) {
            ed.song.metadata.use_preview_file = false;
            ed.song.metadata.preview_loop.start = loop.start;
            ed.song.metadata.preview_loop.duration = loop.duration;
        },
        [&](const std::string& file) {
            ed.song.metadata.use_preview_file = true;
            ed.song.metadata.preview_file = file;
            ed.reload_preview_audio();
        },
    };
    std::visit(set_value_, value);
}


ChangeTiming::ChangeTiming(
    const better::Timing& old_timing,
    const better::Timing& new_timing,
    const TimingOrigin& origin
) :
    ChangeValue(old_timing, new_timing),
    origin(origin)
{
    message = "Change Timing";
}

void ChangeTiming::set_value(EditorState& ed, const better::Timing& value) const {
    const auto set_value_ = VariantVisitor {
        [&](const GlobalTimingObject& g) {
            ed.song.timing = std::make_shared<better::Timing>(value);
        },
        [&](const std::string& chart) {
            ed.song.charts.at(chart).timing = std::make_shared<better::Timing>(value);
        }
    };
    std::visit(set_value_, origin);
    ed.reload_applicable_timing();
    ed.reload_sounds_that_depend_on_timing();
}