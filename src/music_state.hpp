#pragma once

#include <filesystem>

#include "precise_music.hpp"

struct MusicState {
    explicit MusicState(const std::filesystem::path& path) : music(path) {};
    PreciseMusic music;
    int volume = 10;  // 0 -> 10
    void set_volume(int newMusicVolume);
    void volume_up();
    void volume_down();

    int speed = 10;  // 1 -> 20
    void set_speed(int newMusicSpeed);
    void speed_up();
    void speed_down();
};