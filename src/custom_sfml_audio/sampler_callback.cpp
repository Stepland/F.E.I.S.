#include "sampler_callback.hpp"

#include <limits>

std::optional<Slice> compute_sample_slice(
    const std::int64_t buffer_start,
    const std::int64_t buffer_size,
    const std::int64_t sample_start,
    const std::int64_t sample_size,
    const std::optional<std::int64_t> next_sample_start
) {
    const auto buffer_end = buffer_start + buffer_size;
    const auto sample_end = sample_start + sample_size;
    const auto sample_deoverlapped_end = [&](){
        if (next_sample_start.has_value()) {
            return std::min(*next_sample_start, sample_end);
        } else {
            return sample_end;
        }
    }();
    const auto sample_slice_start = std::max(sample_start, buffer_start);
    const auto sample_slice_end = std::min(sample_deoverlapped_end, buffer_end);
    const auto slice_size = sample_slice_end - sample_slice_start;
    const auto slice_start_relative_to_sample_start = sample_slice_start - sample_start;
    const auto slice_start_relative_to_buffer_start = sample_slice_start - buffer_start;
    // all the possible error cases I could think of
    if (
        sample_deoverlapped_end <= buffer_start
        or sample_start >= buffer_end
        or slice_size <= 0
    ) {
        return {};
    } else {
        return Slice{
            .start_in_sample=slice_start_relative_to_sample_start,
            .size=slice_size,
            .start_in_buffer=slice_start_relative_to_buffer_start,
            .ends_in_this_buffer=sample_deoverlapped_end <= buffer_end
        };
    }
}

void copy_sample_at_points(
    const std::shared_ptr<FakePitchedSoundStream::sound_buffer_type>& sample,
    std::span<sf::Int16> output_buffer,
    std::set<std::int64_t>& starting_points,
    std::int64_t buffer_start
) {
    std::ranges::fill(output_buffer, 0);
    for (auto it = starting_points.begin(); it != starting_points.end();) {
        const auto next_sample_start = [&]() -> std::optional<std::int64_t> {
            const auto next = std::next(it);
            if (next == starting_points.end()) {
                return {};
            } else {
                return *next;
            }
        }();
        const auto slice = compute_sample_slice(
            buffer_start,
            static_cast<std::int64_t>(output_buffer.size()),
            *it,
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