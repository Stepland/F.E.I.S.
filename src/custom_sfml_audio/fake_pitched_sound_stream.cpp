#include "fake_pitched_sound_stream.hpp"

#include <memory>

#include <fmt/core.h>
#include <SFML/Audio/SoundBuffer.hpp>

#include "utf8_strings.hpp"

FakePitchedSoundStream::FakePitchedSoundStream(
    const std::filesystem::path& path_to_sample,
    float pitch_
) :
    pitch(pitch_),
    sample(std::make_shared<sf::SoundBuffer>())
{
    if (not sample->loadFromFile(to_utf8_encoded_string(path_to_sample))) {
        throw std::runtime_error(fmt::format("Could not load audio sample : {}", path_to_sample.string()));
    }
    finish_initializing_the_sample();
}

FakePitchedSoundStream::FakePitchedSoundStream(
    std::shared_ptr<sf::SoundBuffer> sample_,
    float pitch_
) :
    pitch(pitch_),
    sample(sample_)
{
    finish_initializing_the_sample();
}

void FakePitchedSoundStream::finish_initializing_the_sample() {
    sf::SoundStream::initialize(sample->getChannelCount(), sample->getSampleRate());
    output_buffer.resize(openAL_time_to_samples(sf::seconds(1)), 0);
}

std::int64_t FakePitchedSoundStream::openAL_time_to_samples(sf::Time position) const {
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep Int64 as the data type throughout the whole operation.
    return ((static_cast<std::int64_t>(position.asMicroseconds()) * sample->getSampleRate() * sample->getChannelCount()) + 500000) / 1000000;
}

sf::Time FakePitchedSoundStream::samples_to_openAL_time(std::int64_t samples) const {
    sf::Time position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (sample->getSampleRate() != 0 && sample->getChannelCount() != 0)
        position = sf::microseconds((samples * 1000000) / (sample->getChannelCount() * sample->getSampleRate()));

    return position;
}

std::int64_t FakePitchedSoundStream::music_time_to_samples(sf::Time position) const {
    return openAL_time_to_samples(position / pitch);
}

sf::Time FakePitchedSoundStream::samples_to_music_time(std::int64_t samples) const {
    return samples_to_openAL_time(samples) * pitch;
}