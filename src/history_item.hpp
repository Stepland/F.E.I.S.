#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "better_notes.hpp"
#include "better_song.hpp"

class EditorState;


class HistoryItem {
public:
    explicit HistoryItem(std::string message = "") :
        message(std::move(message)) {};

    const std::string& get_message() const;
    virtual void do_action(EditorState& ed) const {};
    virtual void undo_action(EditorState& ed) const {};

    virtual ~HistoryItem() = default;

protected:
    std::string message;
};


class AddNotes : public HistoryItem {
public:
    AddNotes(
        const std::string& difficulty_name,
        const better::Notes& notes
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    std::string difficulty_name;
    better::Notes notes;
};


class RemoveNotes : public AddNotes {
public:
    RemoveNotes(
        const std::string& difficulty_name,
        const better::Notes& notes
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;
};


class RerateChart : public HistoryItem {
public:
    RerateChart(
        const std::string& chart,
        const std::optional<Decimal>& old_level,
        const std::optional<Decimal>& new_level
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    std::string chart;
    std::optional<Decimal> old_level;
    std::optional<Decimal> new_level;
};


class RenameChart : public HistoryItem {
public:
    RenameChart(
        const std::string& old_name,
        const std::string& new_name
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    std::string old_name;
    std::string new_name;
};
