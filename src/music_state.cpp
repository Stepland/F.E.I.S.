#include "music_state.hpp"
#include "toolbox.hpp"


void MusicState::set_speed(int newMusicSpeed) {
    speed = std::clamp(newMusicSpeed, 1, 20);
    music.setPitch(speed / 10.f);
}

void MusicState::speed_up() {
    set_speed(speed + 1);
}

void MusicState::speed_down() {
    set_speed(speed - 1);
}

void MusicState::set_volume(int newMusicVolume) {
    volume = std::clamp(newMusicVolume, 0, 10);
    music.setVolume(Toolbox::convertVolumeToNormalizedDB(volume)*100.f);
}

void MusicState::volume_up() {
    set_volume(volume + 1);
}

void MusicState::volume_down() {
    set_volume(volume - 1);
}