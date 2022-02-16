#include "more_precise_music.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

MorePreciseMusic::MorePreciseMusic(const std::filesystem::path& path) {
    if (not alIsExtensionPresent("AL_SOFT_source_latency")) {
        throw std::runtime_error("Error: AL_SOFT_source_latency not supported");
    }

    (alGetSourcedvSOFT) = reinterpret_cast<LPALGETSOURCEDVSOFT>(alGetProcAddress("alGetSourcedvSOFT"));

    if (not this->openFromFile(path.string())) {
        throw std::runtime_error("Could not open "+path.string());
    }
}

bool MorePreciseMusic::onGetData(sf::SoundStream::Chunk& data) {
    auto result = sf::Music::onGetData(data);
    time_since_last_mix.restart();
    ALdouble offsets[2];
    alGetSourcedvSOFT(m_source, AL_SEC_OFFSET_LATENCY_SOFT, offsets);
    open_al_playback_position_from_lag = sf::seconds(offsets[0]);
    open_al_lag = sf::seconds(offsets[1]);
    ALfloat secs = 0.f;
    alGetSourcef(m_source, AL_SEC_OFFSET, &secs);
    open_al_offset = sf::seconds(secs);
    return result;
}

int main(int argc, char** argv) {
    std::vector<std::string> args = {argv, argv+argc};
    if (args.size() != 2) {
        std::cout << "Usage : " << args[0] << " [file]" << std::endl;
        return -1;
    }

    auto music = MorePreciseMusic{args[1]};
    music.play();
    std::cout << "getPlayingOffset, time_since_last_mix, open_al_playback_position_from_lag, open_al_lag, open_al_offset" << std::endl;
    for (int i = 0; i < 5000; i++) {
    //while (music.getStatus() == sf::Music::Playing) {
        std::cout << music.getPlayingOffset().asMicroseconds() << ", ";
        std::cout << music.time_since_last_mix.getElaspedTime().asMicroseconds() << ", ";
        std::cout << music.open_al_playback_position_from_lag.asMicroseconds() << ", ";
        std::cout << music.open_al_lag.asMicroseconds() << ", ";
        std::cout << music.open_al_offset.asMicroseconds() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}