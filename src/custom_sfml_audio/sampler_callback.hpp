#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <span>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Config.hpp>

void copy_sample_at_points(
    const std::shared_ptr<sf::SoundBuffer>& sample,
    std::span<sf::Int16> output_buffer,
    std::set<std::int64_t>& starting_points,
    std::int64_t absolute_buffer_start
);

template<class T>
void copy_sample_at_points(
    const std::shared_ptr<sf::SoundBuffer>& sample,
    std::span<sf::Int16> output_buffer,
    std::map<std::int64_t, T>& starting_points,
    std::int64_t absolute_buffer_start
) {
    std::ranges::fill(output_buffer, 0);
    for (auto it = starting_points.begin(); it != starting_points.end();) {
        const auto absolute_sample_start = it->first;
        const auto absolute_buffer_end = absolute_buffer_start + static_cast<std::int64_t>(output_buffer.size());
        const auto absolute_sample_end = absolute_sample_start + static_cast<std::int64_t>(sample->getSampleCount());
        const auto absolute_sample_deoverlapped_end = std::min(
            absolute_sample_end,
            [&](const auto& it){
                const auto next = std::next(it);
                if (next != starting_points.end()) {
                    return next->first;
                } else {
                    return INT64_MAX;
                }
            }(it)
        );
        const auto absolute_sample_slice_start = std::max(
            absolute_sample_start,
            absolute_buffer_start
        );
        const auto absolute_sample_slice_end = std::min(
            absolute_sample_deoverlapped_end,
            absolute_buffer_end
        );
        const auto slice_size = absolute_sample_slice_end - absolute_sample_slice_start;
        const auto slice_start_relative_to_sample_start = absolute_sample_slice_start - absolute_sample_start;
        const auto slice_start_relative_to_buffer_start = absolute_sample_slice_start - absolute_buffer_start;
        
        // Exit early in all the possible error cases I could think of
        if (
            absolute_sample_deoverlapped_end <= absolute_buffer_start
            or absolute_sample_start >= absolute_buffer_end
            or slice_size <= 0
        ) {
            it = starting_points.erase(it);
            continue;
        }
        
        const auto input_start = sample->getSamples() + slice_start_relative_to_sample_start;
        const auto input_end = input_start + slice_size;
        const auto output_start = output_buffer.begin() + slice_start_relative_to_buffer_start;
        std::copy(
            input_start,
            input_end,
            output_start
        );
        // has this sample been fully played in this buffer ?
        if (absolute_sample_deoverlapped_end <= absolute_buffer_end) {
            it = starting_points.erase(it);
        } else {
            ++it;
        }
    }
}