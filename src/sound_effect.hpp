#pragma once

#include <SFML/Audio.hpp>
#include <filesystem>
#include <iostream>

/*
 * Holds an sf::Sound and can display some controls associated with it (volume
 * and on/off toggle)
 */
class SoundEffect {
public:
    explicit SoundEffect(std::filesystem::path path);
    void play();

    int getVolume() const;
    void setVolume(int volume);
    void volumeUp() { setVolume(volume + 1); };
    void volumeDown() { setVolume(volume - 1); };

    bool shouldPlay;
    bool toggle() {
        shouldPlay = !shouldPlay;
        return shouldPlay;
    };

    void displayControls();

private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
    int volume;
};
