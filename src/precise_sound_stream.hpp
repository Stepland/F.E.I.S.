#pragma once

#include <array>
#include <filesystem>

#include <SFML/Audio/SoundStream.hpp>

#include "AL/al.h"
#include "AL/alext.h"

class PreciseSoundStream : public sf::SoundStream {
public:
    sf::Time getPrecisePlayingOffset() const;
    void play();
protected:
    void initialize_open_al_extension();
    LPALGETSOURCEDVSOFT alGetSourcedvSOFT;
private:
    std::array<sf::Time, 2> alSecOffsetLatencySoft() const;
    sf::Time lag = sf::Time::Zero;
};