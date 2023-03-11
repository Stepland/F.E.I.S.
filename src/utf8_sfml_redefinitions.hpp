#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include "utf8_sfml.hpp"

namespace feis {
    using Music = feis::HoldFileStreamMixin<sf::Music>;
    using InputSoundFile = feis::HoldFileStreamMixin<sf::InputSoundFile>;
    using SoundBuffer = feis::LoadFromPathMixin<sf::SoundBuffer>;
    using Texture = feis::LoadFromPathMixin<sf::Texture>;
}