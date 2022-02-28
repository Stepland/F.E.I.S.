#ifndef FEIS_DELAYED_SOUND_H
#define FEIS_DELAYED_SOUND_H

#include <filesystem>
#include <vector>

#include <SFML/Audio.hpp>

/*
For sounds that need to be played often (at most 16 at a time) and with very
precise delay, useful for claps and note ticks
*/
class DelayedSound {
public:
    explicit DelayedSound(const std::filesystem::path& sound_file);
    void playIn(const sf::Time& time);
    void setVolume(float volume);
private:
    std::vector<sf::Sound> voices;
    sf::SoundBuffer audio;
};

#endif  // FEIS_DELAYED_SOUND_H