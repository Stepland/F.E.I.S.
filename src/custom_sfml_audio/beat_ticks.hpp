#pragma once

#include <set>

#include <SFML/Audio/SoundBuffer.hpp>

#include "../better_timing.hpp"
#include "precise_sound_stream.hpp"

class BeatTicks: public PreciseSoundStream {
public:
    BeatTicks(
        const better::Timing* timing_,
        const std::filesystem::path& assets
    );

    void set_timing(const better::Timing* timing);
    std::atomic<bool> play_chords = true;

protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::vector<sf::Int16> samples;
    std::int64_t current_sample = 0;
    std::int64_t timeToSamples(sf::Time position) const;
    sf::Time samplesToTime(std::int64_t samples) const;

    std::set<std::int64_t> beat_at_sample;

    const better::Timing* timing;
    sf::SoundBuffer beat_tick;
};