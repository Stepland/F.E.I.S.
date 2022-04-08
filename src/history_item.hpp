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

/*
 * A history action replacing every note, typically the first state in history
 */
class OpenChart : public HistoryItem {
public:
    explicit OpenChart(const better::Chart& c, const std::string& difficulty);

    void do_action(EditorState& ed) const override;

protected:
    better::Notes notes;
};

/*
 * Some notes have been toggled, either on or off depending on have_been_added
 */
class AddNotes : public HistoryItem {
public:
    AddNotes(const better::Notes& notes);

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    better::Notes notes;
};

class RemoveNotes : public AddNotes {
public:
    RemoveNotes(const better::Notes& notes);

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;
};

std::string get_message(const std::shared_ptr<HistoryItem>& awm);
