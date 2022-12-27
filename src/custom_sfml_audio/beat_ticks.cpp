#include "beat_ticks.hpp"

#include <SFML/System/Time.hpp>
#include <memory>
#include <stdexcept>

#include "../better_note.hpp"
#include "sampler_callback.hpp"

BeatTicks::BeatTicks(
    const std::shared_ptr<better::Timing>& timing_,
    const std::filesystem::path& assets,
    float pitch_
) :
    FakePitchedSoundStream(assets / "sounds" / "beat.wav", pitch_),
    timing(timing_)
{}

BeatTicks::BeatTicks(
    const std::shared_ptr<better::Timing>& timing_,
    std::shared_ptr<sf::SoundBuffer> beat_tick,
    float pitch_
) :
    FakePitchedSoundStream(beat_tick, pitch_),
    timing(timing_)
{}

std::shared_ptr<BeatTicks> BeatTicks::with_pitch(float new_pitch) {
    return std::make_shared<BeatTicks>(
        timing,
        sample,
        new_pitch
    );
}

std::shared_ptr<BeatTicks> BeatTicks::with_timing(const std::shared_ptr<better::Timing>& timing_) {
    return std::make_shared<BeatTicks>(
        timing_,
        sample,
        pitch
    );
}

bool BeatTicks::onGetData(sf::SoundStream::Chunk& data) {
    if (timing) {
        const auto absolute_buffer_start = first_sample_of_next_buffer;
        const std::int64_t absolute_buffer_end = first_sample_of_next_buffer + static_cast<std::int64_t>(output_buffer.size());
        const auto start_time = samples_to_music_time(absolute_buffer_start);
        const auto end_time = samples_to_music_time(absolute_buffer_end);
        const auto start_beat = timing->beats_at(start_time);
        const auto end_beat = timing->beats_at(end_time);

        auto first_beat = static_cast<std::int64_t>(start_beat);
        while (Fraction{first_beat} < start_beat) {
            first_beat++;
        }
        for (std::int64_t beat = first_beat; Fraction{beat} < end_beat; beat++) {
            const auto time = timing->time_at(Fraction{beat});
            const auto sample = static_cast<std::int64_t>(music_time_to_samples(time));
            beat_at_sample.insert(sample);
        }
        
        copy_sample_at_points(
            sample,
            output_buffer,
            beat_at_sample,
            absolute_buffer_start
        );
    }

    data.samples = output_buffer.data();
    data.sampleCount = output_buffer.size();
    first_sample_of_next_buffer += output_buffer.size();

    return true;
};

void BeatTicks::onSeek(sf::Time timeOffset) {
    first_sample_of_next_buffer = music_time_to_samples(timeOffset);
    beat_at_sample.clear();
};