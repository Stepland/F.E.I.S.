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

    alGetSourcedvSOFT = reinterpret_cast<LPALGETSOURCEDVSOFT>(alGetProcAddress("alGetSourcedvSOFT"));

    if (not this->openFromFile(path.string())) {
        throw std::runtime_error("Could not open "+path.string());
    }
}

std::array<sf::Time, 2> MorePreciseMusic::alSecOffsetLatencySoft() const {
    ALdouble offsets[2];
    alGetSourcedvSOFT(m_source, AL_SEC_OFFSET_LATENCY_SOFT, offsets);
    return {sf::seconds(offsets[0]), sf::seconds(offsets[1])};
}

void MorePreciseMusic::play() {
    sf::Music::play();
    lag = this->alSecOffsetLatencySoft()[1];
}

sf::Time MorePreciseMusic::getPrecisePlayingOffset() const {
    if (this->getStatus() != sf::Music::Playing) {
        return sf::Music::getPlayingOffset();
    } else {
        return sf::Music::getPlayingOffset() - this->alSecOffsetLatencySoft()[1] + lag;
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> args = {argv, argv+argc};
    if (args.size() != 2) {
        std::cout << "Usage : " << args[0] << " [file]" << std::endl;
        return -1;
    }

    auto music = MorePreciseMusic{args[1]};
    music.play();
    std::cout << "normal,Precise" << std::endl;
    while(music.getStatus() == sf::Music::Playing) {
    //for (int i = 0; i < 3000; i++) {
        std::cout << music.getPlayingOffset().asMicroseconds() << ",";
        std::cout << music.getPrecisePlayingOffset().asMicroseconds() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}