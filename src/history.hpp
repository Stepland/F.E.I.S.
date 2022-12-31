#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <stack>
#include <variant>

#include "history_item.hpp"

/*
 *  History implemented this way :
 * 
 *                               last action ->  *  <- back of next_actions
 *                                               *
 *                                               *
 *                                               *  <- front of next_actions
 *        
 *          current song state in the editor ->  o
 *        
 *                    cause of current state ->  *  <- front of previous_actions
 *                                               *
 *                                               *
 *                    first action ever done ->  *  <- back of previous_actions
 *       
 * initial state the file was in when opened ->  x
 */
class History {
    using item = std::shared_ptr<HistoryItem>;
public:
    struct InitialStateSaved {};
    std::optional<item> pop_previous();
    std::optional<item> pop_next();
    void push(const item& elt);
    void display(bool& show);
    void mark_as_saved();
    bool current_state_is_saved() const;
private:
    std::deque<item> previous_actions;
    std::deque<item> next_actions;
    std::optional<std::variant<InitialStateSaved, item>> last_saved_action;
};
