#ifndef FEIS_HISTORY_H
#define FEIS_HISTORY_H

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <stack>

#include <imgui/imgui.h>
#include <variant>

#include "history_item.hpp"

struct InitialStateSaved {};

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
    std::optional<item> pop_previous() {
        if (previous_actions.empty()) {
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

    void display(bool& show) {
        if (ImGui::Begin("History", &show)) {
            for (const auto& it : next_actions | std::views::reverse) {
                ImGui::TextUnformatted(it->get_message().c_str());
                if (last_saved_action and std::holds_alternative<item>(*last_saved_action)) {
                    if (std::get<item>(*last_saved_action) == it) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.3, 0.84,0.08,1), "saved");
                    }
                }
            }
            for (const auto& it : previous_actions) {
                ImGui::TextUnformatted(it->get_message().c_str());
                if (it == *previous_actions.cbegin()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.4, 0.8, 1, 1), "current");
                }
                if (last_saved_action and std::holds_alternative<item>(*last_saved_action)) {
                    if (std::get<item>(*last_saved_action) == it) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.3, 0.84,0.08,1), "saved");
                    }
                }
            }
            ImGui::TextUnformatted("(initial state)");
            if (previous_actions.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.4, 0.8, 1, 1), "current");
            }
            if (last_saved_action and std::holds_alternative<InitialStateSaved>(*last_saved_action)) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3, 0.84,0.08,1), "saved");
            }
        }
        ImGui::End();
    }

    void mark_as_saved() {
        if (not previous_actions.empty()) {
            last_saved_action = previous_actions.front();
        } else {
            last_saved_action = InitialStateSaved{};
        }
    }

    bool current_state_is_saved() const {
        if (not last_saved_action) {
            return false;
        } else {
            const auto is_saved_ = VariantVisitor {
                [&](const InitialStateSaved& i) { return previous_actions.empty(); },
                [&](const item& i) {
                    if (not previous_actions.empty()) {
                        return i == previous_actions.front();
                    } else {
                        return false;
                    }
                }
            };
            return std::visit(is_saved_, *last_saved_action);
        }
    };

private:
    std::deque<item> previous_actions;
    std::deque<item> next_actions;
    std::optional<std::variant<InitialStateSaved, item>> last_saved_action;
};

#endif  // FEIS_HISTORY_H
