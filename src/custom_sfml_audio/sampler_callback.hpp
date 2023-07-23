#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Config.hpp>

#include "fake_pitched_sound_stream.hpp"
#include "fmt/core.h"

struct Slice {
    std::int64_t start_in_sample;
    std::int64_t size;
    std::int64_t start_in_buffer;
    bool ends_in_this_buffer;
};

std::optional<Slice> compute_sample_slice(
    const std::int64_t buffer_start,
    const std::int64_t buffer_size,
    const std::int64_t sample_start,
    const std::int64_t sample_size,
    const std::optional<std::int64_t> next_sample_start
);

void copy_sample_at_points(
    const std::shared_ptr<FakePitchedSoundStream::sound_buffer_type>& sample,
    std::span<sf::Int16> output_buffer,
    std::set<std::int64_t>& starting_points,
    std::int64_t absolute_buffer_start
);

template<class T>
void copy_sample_at_points(
    const std::shared_ptr<FakePitchedSoundStream::sound_buffer_type>& sample,
    std::span<sf::Int16> output_buffer,
    std::map<std::int64_t, T>& starting_points,
    std::int64_t buffer_start
) {
    std::ranges::fill(output_buffer, 0);
    for (auto it = starting_points.begin(); it != starting_points.end();) {
        const auto next_sample_start = [&]() -> std::optional<std::int64_t> {
            const auto next = std::next(it);
            if (next == starting_points.end()) {
                return {};
            } else {
                return next->first;
            }
        }();
        const auto slice = compute_sample_slice(
            buffer_start,
            static_cast<std::int64_t>(output_buffer.size()),
            it->first,
            static_cast<std::int64_t>(sample->getSampleCount()),
            next_sample_start
        );

        // bounds checking failed somehow
        if (not slice) {
            it = starting_points.erase(it);
            continue;
        }
        
        const auto input_start = sample->getSamples() + slice->start_in_sample;
        const auto input_end = input_start + slice->size;
        const auto output_start = output_buffer.begin() + slice->start_in_buffer;
        std::copy(
            input_start,
            input_end,
            output_start
        );
        // has this sample been fully played in this buffer ?
        if (slice->ends_in_this_buffer) {
            it = starting_points.erase(it);
        } else {
            ++it;
        }
    }
}