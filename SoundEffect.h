//
// Created by Syméon on 24/03/2019.
//

#ifndef FEIS_SOUNDEFFECT_H
#define FEIS_SOUNDEFFECT_H

#include <filesystem>
#include <iostream>
#include <SFML/Audio.hpp>

class SoundEffect {
public:
    explicit SoundEffect(std::string filename);
    void play();

    int getVolume() const;
    void setVolume(int volume);
    void volumeUp() {setVolume(volume+1);};
    void volumeDown() {setVolume(volume-1);};

    bool shouldPlay;
    bool toggle() {shouldPlay = !shouldPlay; return shouldPlay;};

    void displayControls();

private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
    int volume;
};


#endif //FEIS_SOUNDEFFECT_H
