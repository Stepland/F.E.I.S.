#pragma once

#include <AL/al.h>
#include <SFML/Audio/SoundStream.hpp>

class OpenSoundStream : public sf::SoundStream {
public:
    ALuint get_source() const;
    bool public_data_callback(Chunk& data);
    void public_seek_callback(sf::Time timeOffset);
    sf::Int64 public_loop_callback();

    int get_volume() const;
    void set_volume(int volume);

private:
    int volume = 10;  // 0 -> 10
};