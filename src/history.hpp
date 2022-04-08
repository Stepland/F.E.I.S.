#ifndef FEIS_HISTORY_H
#define FEIS_HISTORY_H

#include <imgui/imgui.h>

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <stack>

#include "history_item.hpp"

/*
 *  History implemented this way :
 *
 *  last action            ->  *  <- back of next_actions
 *                             *
 *                             *
 *                             *  <- front of next_actions
 *
 *  given state of stuff   ->  o
 *
 *  cause of current state ->  *  <- front of previous_actions
 *                             *
 *                             *
 *  first action ever done ->  *  <- back of previous_actions
 *
 */
class History {
    using item = std::shared_ptr<HistoryItem>;
public:
    /*
     * we cannot undo the very first action, which in F.E.I.S corresponds to
     * opening a chart
     */
    std::optional<item> pop_previous() {
        if (previous_actions.size() == 1) {
            return {};
        } else {
            auto elt = previous_actions.front();
            next_actions.push_front(elt);
            previous_actions.pop_front();
            return elt;
        }
    }

    std::optional<item> pop_next() {
        if (next_actions.empty()) {
            return {};
        } else {
            auto elt = next_actions.front();
            previous_actions.push_front(elt);
            next_actions.pop_front();
            return elt;
        }
    }

    void push(const item& elt) {
        previous_actions.push_front(elt);
        if (not next_actions.empty()) {
            next_actions.clear();
        }
    }

    void display() {
        if (ImGui::Begin("History")) {
            ImGui::Indent();
            for (auto it = next_actions.crbegin(); it != next_actions.crend(); ++it) {
                ImGui::TextUnformatted((*it)->get_message().c_str());
            }
            ImGui::Unindent();
            if (previous_actions.empty()) {
                ImGui::Bullet();
                ImGui::TextDisabled("(empty)");
            } else {
                auto it = previous_actions.cbegin();
                ImGui::Bullet();
                ImGui::TextUnformatted((*it)->get_message().c_str());
                ImGui::Indent();
                ++it;
                while (it != previous_actions.cend()) {
                    ImGui::TextUnformatted((*it)->get_message().c_str());
                    ++it;
                }
                ImGui::Unindent();
            }
        }
        ImGui::End();
    }

    bool empty() { return previous_actions.size() <= 1; }

    void mark_as_saved() {
        if (next_actions.empty()) {
            last_saved_action.reset();
        } else {
            last_saved_action = previous_actions.front(); 
        }
    }

    bool current_state_is_saved() const {
        if (not last_saved_action) {
            return false;
        } else {
            return *last_saved_action == previous_actions.front();
        }
    };

private:
    std::deque<item> previous_actions;
    std::deque<item> next_actions;
    std::optional<item> last_saved_action;
};

#endif  // FEIS_HISTORY_H
