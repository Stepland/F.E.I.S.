#include "waveform_view.hpp"

#include <SFML/System/Vector2.hpp>

#include <imgui.h>
#include <imgui_extras.hpp>
#include <imgui_internal.h>

WaveformView::WaveformView(const std::shared_ptr<OpenMusic>& audio_):
    audio(audio_)
{

}

void WaveformView::draw(const sf::Time current_time) {
    if (ImGui::Begin("Waveform view")) {
        if (not data_is_ready) {
            feis::CenteredText("Loading ...");
            return ImGui::End();
        }

        const auto window = ImGui::GetCurrentWindow();
        const auto work_rect = window->WorkRect;
        auto draw_list = window->DrawList;
        const float cursor_y = 50.f;
        const float waveform_center_x = work_rect.GetCenter().x;
        if (max_of_every_64_samples.size() <= 1) {
            feis::CenteredText("Not enough samples ... Why ?");
            return ImGui::End();
        }

        const std::int64_t sample_at_cursor = audio->timeToSamples(current_time);
        const auto bucket_at_cursor = sample_at_cursor / 64;
        const auto first_bucket = bucket_at_cursor - static_cast<std::int64_t>(cursor_y);
        const auto end_bucket = first_bucket + static_cast<std::int64_t>(work_rect.GetHeight());
        auto it = max_of_every_64_samples.begin();
        auto next_it = std::next(it);
        for (; next_it != max_of_every_64_samples.end(); ++next_it, ++it) {
            
        }
    }
    ImGui::End();
}
