#ifndef FEIS_SOUNDEFFECT_H
#define FEIS_SOUNDEFFECT_H

#include <SFML/Audio.hpp>
#include <filesystem>
#include <iostream>

#include "delayed_sound.hpp"

/*
 * Wraps a DelayedSound with GUI volume control and on/off toggle
 */
class SoundEffect {
public:
    explicit SoundEffect(std::filesystem::path path);
    void playIn(const sf::Time& time);

    int getVolume() const;
    void setVolume(int volume);
    void volumeUp() { setVolume(volume + 1); };
    void volumeDown() { setVolume(volume - 1); };

    bool shouldPlay;
    bool toggle() {
        shouldPlay = not shouldPlay;
        return shouldPlay;
    };

    void displayControls();

private:
    DelayedSound sound;
    int volume;
};

#endif  // FEIS_SOUNDEFFECT_H
