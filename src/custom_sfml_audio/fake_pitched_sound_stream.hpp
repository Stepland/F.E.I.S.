#pragma once

#include <SFML/Audio/SoundBuffer.hpp>

#include "precise_sound_stream.hpp"
#include "../utf8_sfml_redefinitions.hpp"

/*
SoundStream that doesn't pitch-shift but differenciates between the current
offset in the music file played alongside (the music time) and the current
offset it reports to openAL (the OpenAL time)

This allows note claps and beat ticks not to get pitch-shifted when the music
is.
*/
class FakePitchedSoundStream : public PreciseSoundStream {
public:
    using sound_buffer_type = feis::SoundBuffer;
    FakePitchedSoundStream(
        const std::filesystem::path& path_to_sample,
        float pitch_
    );

    FakePitchedSoundStream(
        std::shared_ptr<sound_buffer_type> sample_,
        float pitch_
    );
protected:
    void finish_initializing_the_sample();
    
    float pitch = 1.f;
    std::shared_ptr<sound_buffer_type> sample;
    std::vector<sf::Int16> output_buffer;
    std::int64_t first_sample_of_next_buffer = 0;

    std::int64_t openAL_time_to_samples(sf::Time position) const;
    sf::Time samples_to_openAL_time(std::int64_t samples) const;
    std::int64_t music_time_to_samples(sf::Time position) const;
    sf::Time samples_to_music_time(std::int64_t samples) const;
};