#pragma once

#include <memory>
#include <set>

#include <SFML/Audio/SoundBuffer.hpp>

#include "../better_timing.hpp"
#include "fake_pitched_sound_stream.hpp"

class BeatTicks: public FakePitchedSoundStream {
public:
    BeatTicks(
        const better::Timing* timing_,
        const std::filesystem::path& assets,
        float pitch_
    );

    BeatTicks(
        const better::Timing* timing_,
        std::shared_ptr<sf::SoundBuffer> beat_tick_,
        float pitch_
    );

    void set_timing(const better::Timing* timing);
    
    std::shared_ptr<BeatTicks> with_pitch(float pitch);

protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::set<std::int64_t> beat_at_sample;

    const better::Timing* timing;
};