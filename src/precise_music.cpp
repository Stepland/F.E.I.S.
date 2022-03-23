#include "precise_music.hpp"

PreciseMusic::PreciseMusic(const std::filesystem::path& path) {
    if (not alIsExtensionPresent("AL_SOFT_source_latency")) {
        throw std::runtime_error("Error: AL_SOFT_source_latency not supported");
    }

    alGetSourcedvSOFT = reinterpret_cast<LPALGETSOURCEDVSOFT>(alGetProcAddress("alGetSourcedvSOFT"));

    if (not this->openFromFile(path.string())) {
        throw std::runtime_error("Could not open "+path.string());
    }
}

std::array<sf::Time, 2> PreciseMusic::alSecOffsetLatencySoft() const {
    ALdouble offsets[2];
    alGetSourcedvSOFT(m_source, AL_SEC_OFFSET_LATENCY_SOFT, offsets);
    return {sf::seconds(offsets[0]), sf::seconds(offsets[1])};
}

void PreciseMusic::play() {
    sf::Music::play();
    lag = this->alSecOffsetLatencySoft()[1];
}

sf::Time PreciseMusic::getPrecisePlayingOffset() const {
    if (this->getStatus() != sf::Music::Playing) {
        return sf::Music::getPlayingOffset();
    } else {
        return (
            sf::Music::getPlayingOffset()
            - (this->alSecOffsetLatencySoft()[1] * this->getPitch())
            + (lag * this->getPitch())
        );
    }
}