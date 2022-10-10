#include "open_sound_stream.hpp"

#include <algorithm>

#include "../toolbox.hpp"

ALuint OpenSoundStream::get_source() const {
    return m_source;
}

bool OpenSoundStream::public_data_callback(Chunk& data) {
    return onGetData(data);
}

void OpenSoundStream::public_seek_callback(sf::Time timeOffset) {
    onSeek(timeOffset);
}

sf::Int64 OpenSoundStream::public_loop_callback() {
    return onLoop();
}

int OpenSoundStream::get_volume() const {
    return volume;
}

void OpenSoundStream::set_volume(int volume_) {
    volume = std::clamp(volume_, 0, 10);
    setVolume(Toolbox::convertVolumeToNormalizedDB(volume)*100.f);
}