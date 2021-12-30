#ifndef FEIS_EDITACTIONS_H
#define FEIS_EDITACTIONS_H

#include "notifications_queue.hpp"
#include "editor_state.hpp"

namespace Move {

    void backwardsInTime(std::optional<EditorState>& ed);
    void forwardsInTime(std::optional<EditorState>& ed);

}

namespace Edit {

    void undo(std::optional<EditorState>& ed, NotificationsQueue& nq);
    void redo(std::optional<EditorState>& ed, NotificationsQueue& nq);

    void cut(std::optional<EditorState>& ed, NotificationsQueue& nq);
    void copy(std::optional<EditorState>& ed, NotificationsQueue& nq);
    void paste(std::optional<EditorState>& ed, NotificationsQueue& nq);
    void delete_(std::optional<EditorState>& ed, NotificationsQueue& nq);

};


#endif //FEIS_EDITACTIONS_H
