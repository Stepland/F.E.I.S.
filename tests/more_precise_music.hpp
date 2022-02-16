#include <filesystem>

#include <SFML/Audio.hpp>

#include "AL/al.h"
#include "AL/alext.h"

struct MorePreciseMusic : sf::Music {
    MorePreciseMusic(const std::filesystem::path& path);
    sf::Clock time_since_last_mix;
    sf::Time open_al_playback_position_from_lag = sf::Time::Zero;
    sf::Time open_al_lag = sf::Time::Zero;
    sf::Time open_al_offset = sf::Time::Zero;
    bool lag_measured = false;
    LPALGETSOURCEDVSOFT alGetSourcedvSOFT;
protected:
    bool onGetData(sf::SoundStream::Chunk& data) override;
};