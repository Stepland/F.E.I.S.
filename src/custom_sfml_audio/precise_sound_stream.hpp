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