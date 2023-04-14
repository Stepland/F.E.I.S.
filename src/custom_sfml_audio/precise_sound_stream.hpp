#pragma once

#include <SFML/System/Time.hpp>
#include <array>
#include <filesystem>

#include "AL/al.h"
#include "AL/alext.h"

#include "open_sound_stream.hpp"

struct PreciseSoundStream : public OpenSoundStream {
    PreciseSoundStream();
    sf::Time getPrecisePlayingOffset() const;
    void play();
    void initialize_open_al_extension();
    LPALGETSOURCEDVSOFT alGetSourcedvSOFT;
    std::array<sf::Time, 2> alSecOffsetLatencySoft() const;
    sf::Time lag = sf::Time::Zero;
};

template<class T>
std::int64_t time_to_samples(sf::Time position, T sample_rate, T channel_count) {
    const auto signed_sample_rate = static_cast<std::int64_t>(sample_rate);
    const auto signed_channel_count = static_cast<std::int64_t>(channel_count);
    return ((static_cast<std::int64_t>(position.asMicroseconds()) * signed_sample_rate * signed_channel_count) + 500000) / 1000000;
};

template<class T>
sf::Time samples_to_time(std::int64_t samples, T sample_rate, T channel_count) {
    // Make sure we don't divide by 0
    if (sample_rate != 0 && channel_count != 0) {
        return sf::microseconds((samples * 1000000) / (channel_count * sample_rate));
    } else {
        return sf::Time::Zero;
    }
};