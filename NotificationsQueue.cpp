//
// Created by Syméon on 02/03/2019.
//

#include <imgui.h>
#include "NotificationsQueue.h"

void NotificationsQueue::push(const std::shared_ptr<Notification> &notification) {
    while (queue.size() >= max_size) {
        queue.pop_back();
    }
    queue.push_front(notification);
    last_push.restart();
}

void NotificationsQueue::display() {
    update();
    if (not queue.empty()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
        ImGui::SetNextWindowPos(ImVec2(5,20), ImGuiCond_Always);
        ImGui::Begin(
                "Notifications",
                nullptr,
                ImGuiWindowFlags_NoNav
                |ImGuiWindowFlags_NoDecoration
                |ImGuiWindowFlags_NoInputs
                |ImGuiWindowFlags_NoMove
                |ImGuiWindowFlags_NoBackground
                |ImGuiWindowFlags_AlwaysAutoResize
                |ImGuiWindowFlags_NoFocusOnAppearing
        );
        {
            float alpha = 1.0f;
            if (queue.size() == 1) {
                alpha = time_to_alpha(last_push.getElapsedTime().asSeconds()/4.0f);
            } else {
                alpha = time_to_alpha(last_push.getElapsedTime().asSeconds());
            }
            for (int i = queue.size() - 1; i >= 0; --i) {
                if (i == queue.size()-1) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,alpha);
                    queue[i]->display();
                    ImGui::PopStyleVar();
                } else {
                    queue[i]->display();
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void NotificationsQueue::update() {
    if (queue.size() > 1) {
        if (last_push.getElapsedTime().asSeconds() > 0.5f) {
            queue.pop_back();
            last_push.restart();
        }
    } else if (queue.size() == 1) {
        if (last_push.getElapsedTime().asSeconds() > 2.f) {
            queue.pop_back();
            last_push.restart();
        }
    }
}
