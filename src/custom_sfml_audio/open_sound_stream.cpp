#include "open_sound_stream.hpp"

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