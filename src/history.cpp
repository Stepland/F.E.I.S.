#include "history.hpp"

#include <imgui/imgui.h>
#include "colors.hpp"


std::optional<History::item> History::pop_previous() {
    if (previous_actions.empty()) {
        return {};
    } else {
        auto elt = previous_actions.front();
        next_actions.push_front(elt);
        previous_actions.pop_front();
        return elt;
    }
}

std::optional<History::item> History::pop_next() {
    if (next_actions.empty()) {
        return {};
    } else {
        auto elt = next_actions.front();
        previous_actions.push_front(elt);
        next_actions.pop_front();
        return elt;
    }
}

void History::push(const History::item& elt) {
    previous_actions.push_front(elt);
    if (not next_actions.empty()) {
        next_actions.clear();
    }
}

void History::display(bool& show) {
    if (ImGui::Begin("History", &show)) {
        for (const auto& it : next_actions | std::views::reverse) {
            ImGui::TextUnformatted(it->get_message().c_str());
            if (last_saved_action and std::holds_alternative<item>(*last_saved_action)) {
                if (std::get<item>(*last_saved_action) == it) {
                    ImGui::SameLine();
                    ImGui::TextColored(colors::green, "saved");
                }
            }
        }
        for (const auto& it : previous_actions) {
            ImGui::TextUnformatted(it->get_message().c_str());
            if (it == *previous_actions.cbegin()) {
                ImGui::SameLine();
                ImGui::TextColored(colors::cyan, "current");
            }
            if (last_saved_action and std::holds_alternative<item>(*last_saved_action)) {
                if (std::get<item>(*last_saved_action) == it) {
                    ImGui::SameLine();
                    ImGui::TextColored(colors::green, "saved");
                }
            }
        }
        ImGui::TextUnformatted("(initial state)");
        if (previous_actions.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(colors::cyan, "current");
        }
        if (last_saved_action and std::holds_alternative<InitialStateSaved>(*last_saved_action)) {
            ImGui::SameLine();
            ImGui::TextColored(colors::green, "saved");
        }
    }
    ImGui::End();
}

void History::mark_as_saved() {
    if (not previous_actions.empty()) {
        last_saved_action = previous_actions.front();
    } else {
        last_saved_action = InitialStateSaved{};
    }
}

bool History::current_state_is_saved() const {
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
}
