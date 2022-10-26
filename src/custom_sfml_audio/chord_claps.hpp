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
        const std::shared_ptr<better::Notes>& notes_,
        const std::shared_ptr<better::Timing>& timing_,
        const std::filesystem::path& assets,
        float pitch_
    );

    ChordClaps(
        const std::shared_ptr<better::Notes>& notes_,
        const std::shared_ptr<better::Timing>& timing_,
        std::shared_ptr<sf::SoundBuffer> note_clap_,
        float pitch_
    );

    std::shared_ptr<ChordClaps> with_notes_and_timing(
        const std::shared_ptr<better::Notes>& notes_,
        const std::shared_ptr<better::Timing>& timing_
    );

    std::shared_ptr<ChordClaps> with_pitch(float pitch);

protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::map<std::int64_t, unsigned int> notes_at_sample;

    std::shared_ptr<better::Notes> notes;
    std::shared_ptr<better::Timing> timing;
};