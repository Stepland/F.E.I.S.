#ifndef FEIS_HISTORY_H
#define FEIS_HISTORY_H

#include <functional>
#include <imgui/imgui.h>
#include <optional>
#include <stack>

/*
 *  History implemented this way :
 *
 *  last action            ->  *  <- back of next_actions
 *                             *
 *                             *
 *                             *  <- front of next_actions
 *
 *  given state of stuff   ->  x
 *
 *  cause of current state ->  *  <- front of previous_actions
 *                             *
 *                             *
 *  first action ever done ->  *  <- back of previous_actions
 *
 *
 */
template<typename T>
class History {
public:
    /*
     * we cannot undo the very first action, which in F.E.I.S corresponds to
     * opening a chart
     */
    std::optional<T> get_previous() {
        if (previous_actions.size() == 1) {
            return {};
        } else {
            T elt = previous_actions.front();
            next_actions.push_front(elt);
            previous_actions.pop_front();
            return elt;
        }
    }

    std::optional<T> get_next() {
        if (next_actions.empty()) {
            return {};
        } else {
            T elt = next_actions.front();
            previous_actions.push_front(elt);
            next_actions.pop_front();
            return elt;
        }
    }

    void push(const T& elt) {
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

private:
    std::deque<T> previous_actions;
    std::deque<T> next_actions;
};

#endif  // FEIS_HISTORY_H
