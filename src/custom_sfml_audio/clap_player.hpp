#pragma once

#include <map>
#include <memory>
#include <set>

#include <SFML/Audio/SoundBuffer.hpp>

#include "../better_notes.hpp"
#include "../better_timing.hpp"
#include "open_sound_stream.hpp"

class ClapPlayer: public OpenSoundStream {
public:
    ClapPlayer(
        const better::Notes* notes_,
        const better::Timing* timing_,
        const sf::SoundBuffer& note_clap_,
        const sf::SoundBuffer& chord_clap_
    );

    void set_notes_and_timing(const better::Notes* notes, const better::Timing* timing);

protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::vector<sf::Int16> samples;
    std::int64_t current_sample = 0;
    std::int64_t timeToSamples(sf::Time position) const;
    sf::Time samplesToTime(std::int64_t samples) const;

    std::map<std::int64_t, unsigned int> notes_at_sample;

    const better::Notes* notes;
    const better::Timing* timing;
    const sf::SoundBuffer& note_clap;
    const sf::SoundBuffer& chord_clap;
};