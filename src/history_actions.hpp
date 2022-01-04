

#ifndef FEIS_HISTORYSTATE_H
#define FEIS_HISTORYSTATE_H

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "chart.hpp"

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
    explicit OpenChart(Chart c);

    void doAction(EditorState& ed) const override;

protected:
    const std::set<Note> notes;
};

/*
 * Some notes have been toggled, either on or off depending on have_been_added
 */
class ToggledNotes : public ActionWithMessage {
public:
    ToggledNotes(std::set<Note> notes, bool have_been_added);

    void doAction(EditorState& ed) const override;
    void undoAction(EditorState& ed) const override;

protected:
    const bool have_been_added;
    const std::set<Note> notes;
};

std::string get_message(const std::shared_ptr<ActionWithMessage>& awm);

#endif  // FEIS_HISTORYSTATE_H
