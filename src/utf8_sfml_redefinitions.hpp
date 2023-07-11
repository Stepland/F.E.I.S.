#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "utf8_sfml.hpp"

namespace feis {
    using Music = feis::UTF8Streamer<sf::Music>;
    using InputSoundFile = feis::UTF8Streamer<sf::InputSoundFile>;
    using SoundBuffer = feis::UTF8Loader<sf::SoundBuffer>;
    using Texture = feis::UTF8Loader<sf::Texture>;
}