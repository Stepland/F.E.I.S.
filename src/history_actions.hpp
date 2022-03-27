#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "better_notes.hpp"
#include "better_song.hpp"

class EditorState;

/*
 * Base class for history actions (stuff that is stored inside of History
 * objects)
 */
class ActionWithMessage {
public:
    explicit ActionWithMessage(std::string message = "") :
        message(std::move(message)) {};

    const std::string& getMessage() const;
    virtual void doAction(EditorState& ed) const {};
    virtual void undoAction(EditorState& ed) const {};

    virtual ~ActionWithMessage() = default;

protected:
    std::string message;
};

/*
 * A history action replacing every note, typically the first state in history
 */
class OpenChart : public ActionWithMessage {
public:
    explicit OpenChart(better::Chart c, const std::string& difficulty);

    void doAction(EditorState& ed) const override;

protected:
    better::Notes notes;
};

/*
 * Some notes have been toggled, either on or off depending on have_been_added
 */
class AddNotes : public ActionWithMessage {
public:
    AddNotes(const better::Notes& notes);

    void doAction(EditorState& ed) const override;
    void undoAction(EditorState& ed) const override;

protected:
    better::Notes notes;
};

class RemoveNotes : public AddNotes {
public:
    RemoveNotes(const better::Notes& notes);

    void doAction(EditorState& ed) const override;
    void undoAction(EditorState& ed) const override;
};

std::string get_message(const std::shared_ptr<ActionWithMessage>& awm);
