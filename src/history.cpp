#include "history.hpp"

#include <imgui/imgui.h>

#include "colors.hpp"
#include "imgui_extras.hpp"

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
    const auto dot_columns_width = 60.f;
    const auto centered_dot = [&](const sf::Color& c){
        const auto sz = ImGui::GetTextLineHeight();
        const auto pos = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(pos +  (dot_columns_width / 2.f) - (sz / 2.f));
        feis::ColorDot(c);
    };
    if (ImGui::Begin("History", &show)) {
        if (ImGui::BeginTable("History", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 60.f);
            ImGui::TableSetupColumn("Saved", ImGuiTableColumnFlags_WidthFixed, 60.f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            for (const auto& it : next_actions | std::views::reverse) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(it->get_message().c_str());
                if (last_saved_action and std::holds_alternative<item>(*last_saved_action)) {
                    if (std::get<item>(*last_saved_action) == it) {
                        ImGui::TableSetColumnIndex(1);
                        centered_dot(colors::green);
                    }
                }
            }

            for (const auto& it : previous_actions) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(it->get_message().c_str());
                if (it == *previous_actions.cbegin()) {
                    ImGui::TableSetColumnIndex(0);
                    centered_dot(colors::cyan);
                }
                if (last_saved_action and std::holds_alternative<item>(*last_saved_action)) {
                    if (std::get<item>(*last_saved_action) == it) {
                        ImGui::TableSetColumnIndex(1);
                        centered_dot(colors::green);
                    }
                }
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted("(initial state)");
            if (previous_actions.empty()) {
                ImGui::TableSetColumnIndex(0);
                centered_dot(colors::cyan);
            }
            if (last_saved_action
                and std::holds_alternative<InitialStateSaved>(*last_saved_action)) {
                ImGui::TableSetColumnIndex(1);
                centered_dot(colors::green);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void History::mark_as_saved() {
    if (not previous_actions.empty()) {
        last_saved_action = previous_actions.front();
    } else {
        last_saved_action = InitialStateSaved {};
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
            }};
        return std::visit(is_saved_, *last_saved_action);
    }
}
