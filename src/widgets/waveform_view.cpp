#include "waveform_view.hpp"

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <imgui.h>
#include <imgui_extras.hpp>
#include <imgui_internal.h>
#include <limits>
#include "toolbox.hpp"
#include "utf8_file_input_stream.hpp"
#include "utf8_sfml.hpp"

WaveformView::WaveformView(const std::filesystem::path& file) {
    sound_file.open_from_path(file);
    worker = std::jthread{&WaveformView::prepare_data, this};
}

void WaveformView::draw(const sf::Time current_time) {
    if (ImGui::Begin("Waveform view")) {
        if (not data_is_ready) {
            feis::CenteredText("Loading ...");
            return ImGui::End();
        }

        const auto channels_it = channels_per_chunk_size.begin();
        if (channels_it == channels_per_chunk_size.end()) {
            feis::CenteredText("No data ???");
            return ImGui::End();
        }

        const auto& [chunk_size, channels] = *channels_it;
        const auto window = ImGui::GetCurrentWindow();
        const auto work_rect = window->WorkRect;
        auto draw_list = window->DrawList;
        const float waveform_w_margin = 10.f;
        const float waveform_bounding_width = work_rect.GetWidth() / channels.size();
        const float waveform_width = waveform_bounding_width - waveform_w_margin;
        const AffineTransform<float> value_to_pixel_offset_from_waveform_center{
            std::numeric_limits<DataPoint::value_type>::min(),
            std::numeric_limits<DataPoint::value_type>::max(),
            -waveform_width / 2,
            waveform_width / 2
        };
        const float cursor_y = 50.f;
        for (std::size_t channel_index = 0; channel_index < channels.size(); channel_index++) {
            const auto& data_points = channels[channel_index];
            const std::int64_t sample_at_cursor = time_to_samples(current_time, sound_file.getSampleRate(), sound_file.getChannelCount());
            const auto chunk_at_cursor = sample_at_cursor / chunk_size / sound_file.getChannelCount();
            const auto first_chunk = chunk_at_cursor - static_cast<std::int64_t>(cursor_y);
            const auto end_chunk = first_chunk + static_cast<std::int64_t>(work_rect.GetHeight());
            const auto waveform_x_center = channel_index * waveform_bounding_width + waveform_bounding_width / 2;
            for (std::int64_t data_point_index = first_chunk; data_point_index < end_chunk; data_point_index++) {
                if (data_point_index < 0 or static_cast<std::size_t>(data_point_index) >= data_points.size()) {
                    continue;
                }
                const auto& data_point = data_points[data_point_index];
                const auto y = work_rect.Min.y + data_point_index - first_chunk;
                const auto x_offset_min = value_to_pixel_offset_from_waveform_center.transform(data_point.min);
                const auto x_offset_max = value_to_pixel_offset_from_waveform_center.transform(data_point.max);
                const auto x_min = work_rect.Min.x + waveform_x_center + x_offset_min;
                const auto x_max = work_rect.Min.x + waveform_x_center + x_offset_max;
                draw_list->AddLine({x_min, y}, {x_max, y}, ImColor(sf::Color::White));
            }
        }
    }
    ImGui::End();
}

void WaveformView::prepare_data() {

    const std::size_t chunk_size = 64 * sound_file.getChannelCount();
    const std::size_t point_count = (sound_file.getSampleCount() / sound_file.getChannelCount() / 64) + 1;
    Channels summary{sound_file.getChannelCount(), DataFrame{point_count}};

    std::vector<sf::Int16> samples;
    samples.resize(chunk_size);
    for (std::size_t point_index = 0; point_index < point_count; point_index++) {
        const auto samples_read = sound_file.read(samples.data(), chunk_size);
        if (samples_read < chunk_size) {
            // looks like we are done reading the file ?
            break;
        }
        for (std::size_t channel_index = 0; channel_index < summary.size(); channel_index++) {
            auto& point = summary[channel_index][point_index];
            point.max = std::numeric_limits<DataPoint::value_type>::min();
            point.min = std::numeric_limits<DataPoint::value_type>::max();
            for (std::size_t sample_index = 0; sample_index < 64; sample_index++) {
                auto& sample = samples[sample_index * sound_file.getChannelCount() + channel_index];
                if (sample > point.max) {
                    point.max = sample;
                }
                if (sample < point.min) {
                    point.min = sample;
                }
            }
        }
        std::ranges::fill(samples, 0);
    }
    channels_per_chunk_size[64] = summary;
    data_is_ready = true;
}
