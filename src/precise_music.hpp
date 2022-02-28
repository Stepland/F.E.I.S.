#ifndef FEIS_PRECISE_MUSIC_H
#define FEIS_PRECISE_MUSIC_H

#include <filesystem>
#include <array>

#include <SFML/Audio.hpp>

#include "AL/al.h"
#include "AL/alext.h"

struct PreciseMusic : sf::Music {
    PreciseMusic(const std::filesystem::path& path);
    std::array<sf::Time, 2> alSecOffsetLatencySoft() const;
    sf::Time getPrecisePlayingOffset() const;
    sf::Time lag = sf::Time::Zero;
    void play();
protected:
    LPALGETSOURCEDVSOFT alGetSourcedvSOFT;
};

#endif  // FEIS_PRECISE_MUSIC_H