#pragma once

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
sf::Uint64 timeToSamples(sf::Time position, T sample_rate, T channel_count) {
    return ((static_cast<sf::Uint64>(position.asMicroseconds()) * sample_rate * channel_count) + 500000) / 1000000;
};