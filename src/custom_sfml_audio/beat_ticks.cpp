#include "beat_ticks.hpp"

#include <SFML/System/Time.hpp>
#include <memory>
#include <stdexcept>

#include "../better_note.hpp"

BeatTicks::BeatTicks(
    const better::Timing* timing_,
    const std::filesystem::path& assets,
    float pitch_
) :
    pitch(pitch_),
    timing(timing_),
    beat_tick(std::make_shared<sf::SoundBuffer>())
{
    if (not beat_tick->loadFromFile(assets / "sounds" / "beat.wav")) {
        throw std::runtime_error("Could not load beat tick audio file");
    }
    sf::SoundStream::initialize(beat_tick->getChannelCount(), beat_tick->getSampleRate());
    samples.resize(timeToSamples(sf::seconds(1)), 0);
}

BeatTicks::BeatTicks(
    const better::Timing* timing_,
    std::shared_ptr<sf::SoundBuffer> beat_tick_,
    float pitch_
) :
    pitch(pitch_),
    timing(timing_),
    beat_tick(beat_tick_)
{
    sf::SoundStream::initialize(beat_tick_->getChannelCount(), beat_tick_->getSampleRate());
    samples.resize(timeToSamples(sf::seconds(1)), 0);
}

void BeatTicks::set_timing(const better::Timing* timing_) {
    timing = timing_;
}

std::shared_ptr<BeatTicks> BeatTicks::with_pitch(float pitch) {
    return std::make_shared<BeatTicks>(
        timing,
        beat_tick,
        pitch
    );
}

bool BeatTicks::onGetData(sf::SoundStream::Chunk& data) {
    samples.assign(samples.size(), 0);
    if (timing != nullptr) {
        const auto start_sample = current_sample;
        const auto end_sample = current_sample + static_cast<std::int64_t>(samples.size());
        const auto start_time = samplesToTime(start_sample);
        const auto end_time = samplesToTime(end_sample);
        const auto start_beat = timing->beats_at(start_time);
        const auto end_beat = timing->beats_at(end_time);

        auto first_beat = static_cast<std::int64_t>(start_beat);
        while (first_beat < start_beat) {
            first_beat++;
        }
        for (std::int64_t beat = first_beat; beat < end_beat; beat++) {
            const auto time = timing->time_at(beat);
            const auto sample = static_cast<std::int64_t>(timeToSamples(time));
            beat_at_sample.insert(sample);
        }

        for (auto it = beat_at_sample.begin(); it != beat_at_sample.end();) {
            // Should we still be playing the clap ?
            const auto next = std::next(it);
            const auto last_audible_start = start_sample - static_cast<std::int64_t>(beat_tick->getSampleCount());
            if (*it <= last_audible_start) {
                it = beat_at_sample.erase(it);
            } else {
                const auto full_tick_start_in_buffer = *it - static_cast<std::int64_t>(start_sample);
                const auto slice_start_in_buffer = std::max(std::int64_t(0), full_tick_start_in_buffer);
                const auto full_tick_end_in_buffer = full_tick_start_in_buffer + static_cast<std::int64_t>(beat_tick->getSampleCount());
                auto slice_end_in_buffer = full_tick_end_in_buffer;
                bool tick_finished_playing_in_current_buffer = true;
                if (next != beat_at_sample.end()) {
                    slice_end_in_buffer = std::min(
                        slice_end_in_buffer,
                        *next - static_cast<std::int64_t>(start_sample)
                    );
                } else if (slice_end_in_buffer > static_cast<std::int64_t>(samples.size())) {
                    tick_finished_playing_in_current_buffer = false;
                    slice_end_in_buffer = static_cast<std::int64_t>(samples.size());
                }
                auto slice_start_in_tick = slice_start_in_buffer - full_tick_start_in_buffer;
                auto slice_size = std::min(
                    slice_end_in_buffer - slice_start_in_buffer,
                    static_cast<std::int64_t>(beat_tick->getSampleCount()) - slice_start_in_tick
                );
                const auto tick_pointer = beat_tick->getSamples() + slice_start_in_tick;
                std::copy(
                    tick_pointer,
                    tick_pointer + slice_size,
                    samples.begin() + slice_start_in_buffer
                );
                if (tick_finished_playing_in_current_buffer) {
                    it = beat_at_sample.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    data.samples = samples.data();
    data.sampleCount = samples.size();
    current_sample += samples.size();

    return true;
};

void BeatTicks::onSeek(sf::Time timeOffset) {
    current_sample = timeToSamples(timeOffset);
    beat_at_sample.clear();
};

std::int64_t BeatTicks::timeToSamples(sf::Time position) const {
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep Int64 as the data type throughout the whole operation.
    return ((static_cast<std::int64_t>((position / pitch).asMicroseconds()) * beat_tick->getSampleRate() * beat_tick->getChannelCount()) + 500000) / 1000000;
}

sf::Time BeatTicks::samplesToTime(std::int64_t samples) const {
    sf::Time position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (beat_tick->getSampleRate() != 0 && beat_tick->getChannelCount() != 0)
        position = sf::microseconds((samples * 1000000) / (beat_tick->getChannelCount() * beat_tick->getSampleRate()));

    return position * pitch;
}