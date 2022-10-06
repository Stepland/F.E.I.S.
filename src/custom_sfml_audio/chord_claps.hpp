#pragma once

#include <map>
#include <memory>

#include <SFML/Audio/SoundBuffer.hpp>

#include "../better_notes.hpp"
#include "../better_timing.hpp"
#include "fake_pitched_sound_stream.hpp"

class ChordClaps: public FakePitchedSoundStream {
public:
    ChordClaps(
        const better::Notes* notes_,
        const better::Timing* timing_,
        const std::filesystem::path& assets,
        float pitch_
    );

    ChordClaps(
        const better::Notes* notes_,
        const better::Timing* timing_,
        std::shared_ptr<sf::SoundBuffer> note_clap_,
        float pitch_
    );

    void set_notes_and_timing(const better::Notes* notes, const better::Timing* timing);

    std::shared_ptr<ChordClaps> with_pitch(float pitch);

protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::map<std::int64_t, unsigned int> notes_at_sample;

    const better::Notes* notes;
    const better::Timing* timing;
};