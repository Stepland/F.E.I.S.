#include <SFML/Audio/SoundStream.hpp>
#include "precise_sound_stream.hpp"

void PreciseSoundStream::initialize_open_al_extension() {
    if (not alIsExtensionPresent("AL_SOFT_source_latency")) {
        throw std::runtime_error("Error: AL_SOFT_source_latency not supported");
    }

    alGetSourcedvSOFT = reinterpret_cast<LPALGETSOURCEDVSOFT>(alGetProcAddress("alGetSourcedvSOFT"));
}

std::array<sf::Time, 2> PreciseSoundStream::alSecOffsetLatencySoft() const {
    ALdouble offsets[2];
    alGetSourcedvSOFT(m_source, AL_SEC_OFFSET_LATENCY_SOFT, offsets);
    return {sf::seconds(offsets[0]), sf::seconds(offsets[1])};
}

void PreciseSoundStream::play() {
    sf::SoundStream::play();
    lag = alSecOffsetLatencySoft()[1];
}

sf::Time PreciseSoundStream::getPrecisePlayingOffset() const {
    if (getStatus() != sf::SoundStream::Playing) {
        return sf::SoundStream::getPlayingOffset();
    } else {
        return (
            sf::SoundStream::getPlayingOffset()
            - (alSecOffsetLatencySoft()[1] * getPitch())
            + (lag * getPitch())
        );
    }
}