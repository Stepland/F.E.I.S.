#pragma once

#include <cstdint>
#include <filesystem>
#include <map>

#include <SFML/Audio.hpp>
#include <SFML/Config.hpp>

#include "better_notes.hpp"
#include "better_timing.hpp"
#include "src/precise_sound_stream.hpp"

class NoteClaps: public PreciseSoundStream {
public:
    NoteClaps(const std::filesystem::path& assets);
    void set_notes_and_timing(const better::Notes* notes, const better::Timing* timing);
protected:
    bool onGetData(sf::SoundStream::Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

    std::int64_t timeToSamples(sf::Time position) const;
    sf::Time samplesToTime(std::int64_t samples) const;

    const better::Notes* notes = nullptr;
    const better::Timing* timing = nullptr;
private:
    sf::SoundBuffer clap;
    std::vector<sf::Int16> output_buffer;
    
    std::int64_t current_sample = 0;
    std::map<std::int64_t, unsigned int> notes_at_sample;
};