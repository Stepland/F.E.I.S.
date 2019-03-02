#include <utility>

//
// Created by Syméon on 02/03/2019.
//

#ifndef FEIS_HISTORYSTATE_H
#define FEIS_HISTORYSTATE_H


#include <string>
#include <variant>
#include <memory>
#include "Chart.h"

class EditorState;

class ActionWithMessage {
public:
    explicit ActionWithMessage(std::string message = "") : message(std::move(message)) {};

    const std::string &getMessage() const;
    virtual void doAction(EditorState &ed) {};
    virtual void undoAction(EditorState &ed) {};

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

    void doAction(EditorState &ed) override;

protected:
    std::set<Note> notes;
};

/*
 * Some notes have been toggled, either on or off depending on have_been_added
 */
class ToggledNotes : public ActionWithMessage {
public:
    ToggledNotes(std::set<Note> notes, bool have_been_added);

    void doAction(EditorState &ed) override;
    void undoAction(EditorState &ed) override;

protected:
    bool have_been_added;
    std::set<Note> notes;
};

auto print_history_message = [](std::shared_ptr<ActionWithMessage> hs) -> std::string {
    return (*hs).getMessage();
};


#endif //FEIS_HISTORYSTATE_H
