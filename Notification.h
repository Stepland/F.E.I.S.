//
// Created by Syméon on 02/03/2019.
//

#ifndef FEIS_NOTIFICATION_H
#define FEIS_NOTIFICATION_H


#include <string>
#include "HistoryActions.h"

class Notification {
public:
    virtual void display() const = 0;

    virtual ~Notification() = default;
};


class TextNotification : public Notification {
public:
    explicit TextNotification(const std::string &message);

    void display() const override;

    const std::string message;
};

class UndoNotification : public Notification {
public:
    explicit UndoNotification(const ActionWithMessage& awm) : message(awm.getMessage()) {};

    void display() const override;

    const std::string message;
};

class RedoNotification : public  Notification {
public:
    explicit RedoNotification(const ActionWithMessage& awm) : message(awm.getMessage()) {};

    void display() const override;

    const std::string message;
};

#endif //FEIS_NOTIFICATION_H
