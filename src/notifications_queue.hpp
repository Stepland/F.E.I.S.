#ifndef FEIS_NOTIFICATIONSQUEUE_H
#define FEIS_NOTIFICATIONSQUEUE_H

#include <SFML/System.hpp>
#include <deque>

#include "notification.hpp"

/*
 * Responsible for displaying the notifications with a fadeout effect
 */
class NotificationsQueue {
public:
    explicit NotificationsQueue(int max_size = 10) : max_size(max_size) {};

    void push(const std::shared_ptr<Notification>& notification);

    void display();

private:
    void update();
    float time_to_alpha(float seconds) {
        return std::max(0.0f, 2.0f * (0.5f - seconds));
    }
    sf::Clock last_push;
    const int max_size;
    std::deque<std::shared_ptr<Notification>> queue;
};

#endif  // FEIS_NOTIFICATIONSQUEUE_H
