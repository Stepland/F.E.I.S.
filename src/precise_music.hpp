#pragma once

#include <filesystem>
#include <array>

#include <SFML/Audio.hpp>

#include "AL/al.h"
#include "AL/alext.h"

struct PreciseMusic : sf::Music {
    explicit PreciseMusic(const std::filesystem::path& path);
    std::array<sf::Time, 2> alSecOffsetLatencySoft() const;
    sf::Time getPrecisePlayingOffset() const;
    sf::Time lag = sf::Time::Zero;
    void play();
protected:
    LPALGETSOURCEDVSOFT alGetSourcedvSOFT;
};