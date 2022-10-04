#pragma once

#include <map>
#include <memory>
#include <set>

#include <SFML/Audio/SoundBuffer.hpp>

#include "../better_notes.hpp"
#include "../better_timing.hpp"
#include "fake_pitched_sound_stream.hpp"
#include "precise_sound_stream.hpp"

class NoteClaps: public FakePitchedSoundStream {
public:
    NoteClaps(
        const better::Notes* notes_,
        const better::Timing* timing_,
        const std::filesystem::path& assets,
        float pitch_
    );

    NoteClaps(
        const better::Notes* notes_,
        const better::Timing* timing_,
        std::shared_ptr<sf::SoundBuffer> note_clap_,
        float time_factor_
    );

    void set_notes_and_timing(const better::Notes* notes, const better::Timing* timing);

    std::shared_ptr<NoteClaps> with_pitch(float pitch);

protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::set<std::int64_t> notes_at_sample;

    const better::Notes* notes;
    const better::Timing* timing;
};