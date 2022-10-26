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
        const std::shared_ptr<better::Notes>& notes_,
        const std::shared_ptr<better::Timing>& timing_,
        const std::filesystem::path& assets,
        float pitch_,
        bool play_chords = true,
        bool play_long_note_ends = false
    );

    NoteClaps(
        const std::shared_ptr<better::Notes>& notes_,
        const std::shared_ptr<better::Timing>& timing_,
        std::shared_ptr<sf::SoundBuffer> note_clap_,
        float pitch_,
        bool play_chords = true,
        bool play_long_note_ends = false
    );

    std::shared_ptr<NoteClaps> with_pitch(float pitch);

    bool does_play_chords() const {return play_chords;};
    std::shared_ptr<NoteClaps> with_chords(bool play_chords);

    bool does_play_long_note_ends() const {return play_long_note_ends;};
    std::shared_ptr<NoteClaps> with_long_note_ends(bool play_long_note_ends);

    std::shared_ptr<NoteClaps> with_params(
        float pitch,
        bool play_chords,
        bool play_long_note_ends
    );

    std::shared_ptr<NoteClaps> with_notes_and_timing(
        const std::shared_ptr<better::Notes>& notes_,
        const std::shared_ptr<better::Timing>& timing_
    );
protected:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::map<std::int64_t, unsigned int> notes_at_sample;
    std::set<std::int64_t> long_note_ends;
    bool play_chords = true;
    bool play_long_note_ends = false;

    std::shared_ptr<better::Notes> notes;
    std::shared_ptr<better::Timing> timing;
};