#ifndef FEIS_NOTIFICATION_H
#define FEIS_NOTIFICATION_H

#include <string>

#include "history_actions.hpp"

/*
 * The display function should call ImGui primitives to display arbitrary stuff
 * in the notifications queue
 */
class Notification {
public:
    virtual void display() const = 0;

    virtual ~Notification() = default;
};

/*
 * Displays the string given to the constructor
 */
class TextNotification : public Notification {
public:
    explicit TextNotification(const std::string& message);

    void display() const override;

    const std::string message;
};

/*
 * Displays "Undo" in orange followed by the message associated with the action
 * passed to the constructor
 */
class UndoNotification : public Notification {
public:
    explicit UndoNotification(const ActionWithMessage& awm) :
        message(awm.getMessage()) {};

    void display() const override;

    const std::string message;
};

/*
 * Displays "Redo" in blue followed by the message associated with the action
 * passed to the constructor
 */
class RedoNotification : public Notification {
public:
    explicit RedoNotification(const ActionWithMessage& awm) :
        message(awm.getMessage()) {};

    void display() const override;

    const std::string message;
};

#endif  // FEIS_NOTIFICATION_H
