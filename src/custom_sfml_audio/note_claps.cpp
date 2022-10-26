#include "note_claps.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>

#include "../better_note.hpp"
#include "src/custom_sfml_audio/fake_pitched_sound_stream.hpp"
#include "src/custom_sfml_audio/sampler_callback.hpp"
#include "src/special_numeric_types.hpp"

NoteClaps::NoteClaps(
    const std::shared_ptr<better::Notes>& notes_,
    const std::shared_ptr<better::Timing>& timing_,
    const std::filesystem::path& assets,
    float pitch_,
    bool play_chords_,
    bool play_long_note_ends_
) :
    FakePitchedSoundStream(assets / "sounds" / "note.wav", pitch_),
    play_chords(play_chords_),
    play_long_note_ends(play_long_note_ends_),
    notes(notes_),
    timing(timing_)
{}

NoteClaps::NoteClaps(
    const std::shared_ptr<better::Notes>& notes_,
    const std::shared_ptr<better::Timing>& timing_,
    std::shared_ptr<sf::SoundBuffer> note_clap,
    float pitch,
    bool play_chords_,
    bool play_long_note_ends_
) :
    FakePitchedSoundStream(note_clap, pitch),
    play_chords(play_chords_),
    play_long_note_ends(play_long_note_ends_),
    notes(notes_),
    timing(timing_)
{}

std::shared_ptr<NoteClaps> NoteClaps::with_pitch(float new_pitch) {
    return std::make_shared<NoteClaps>(
        notes,
        timing,
        sample,
        new_pitch,
        play_chords,
        play_long_note_ends
    );
}

std::shared_ptr<NoteClaps> NoteClaps::with_chords(bool new_play_chords) {
    return std::make_shared<NoteClaps>(
        notes,
        timing,
        sample,
        pitch,
        new_play_chords,
        play_long_note_ends
    );
}

std::shared_ptr<NoteClaps> NoteClaps::with_long_note_ends(bool new_play_long_note_ends) {
    return std::make_shared<NoteClaps>(
        notes,
        timing,
        sample,
        pitch,
        play_chords,
        new_play_long_note_ends
    );
}

std::shared_ptr<NoteClaps> NoteClaps::with_params(
    float pitch_,
    bool play_chords_,
    bool play_long_note_ends_
) {
    return std::make_shared<NoteClaps>(
        notes,
        timing,
        sample,
        pitch_,
        play_chords_,
        play_long_note_ends_
    );
}

std::shared_ptr<NoteClaps> NoteClaps::with_notes_and_timing(
    const std::shared_ptr<better::Notes>& notes_,
    const std::shared_ptr<better::Timing>& timing_
) {
    return std::make_shared<NoteClaps>(
        notes_,
        timing_,
        sample,
        pitch,
        play_chords,
        play_long_note_ends
    );
}

bool NoteClaps::onGetData(sf::SoundStream::Chunk& data) {
    if (timing and notes) {
        long_note_ends.clear();
        const auto absolute_buffer_start = first_sample_of_next_buffer;
        const std::int64_t absolute_buffer_end = first_sample_of_next_buffer + static_cast<std::int64_t>(output_buffer.size());
        const auto start_time = samples_to_music_time(absolute_buffer_start);
        const auto end_time = samples_to_music_time(absolute_buffer_end);
        const auto start_beat = timing->beats_at(start_time);
        const auto end_beat = timing->beats_at(end_time);
        const auto count_clap_at = [&](const Fraction beat){
            const auto time = timing->time_at(beat); 
            const auto sample = static_cast<std::int64_t>(music_time_to_samples(time));
            // we don't want claps that *start* at the end sample since
            // absolute_buffer_end is an *exculsive* end
            if (sample < absolute_buffer_end) {
                notes_at_sample[sample] += 1;
            }
        };

        const auto add_long_note_end = [&](const Fraction beat){
            const auto time = timing->time_at(beat); 
            const auto sample = static_cast<std::int64_t>(music_time_to_samples(time));
            // we don't want claps that *start* at the end sample since
            // absolute_buffer_end is an *exculsive* end
            if (sample < absolute_buffer_end) {
                long_note_ends.insert(sample);
            }
        };

        const auto add_claps_of_note = VariantVisitor {
            [&](const better::TapNote& t) {
                count_clap_at(t.get_time());
            },
            [&](const better::LongNote& l) {
                count_clap_at(l.get_time());
                if (play_long_note_ends) {
                    add_long_note_end(l.get_end());
                }
            },
        };

        notes->in(start_beat, end_beat, [&](const better::Notes::const_iterator& it){
            it->second.visit(add_claps_of_note);
        });
        if (not play_chords) {
            std::erase_if(notes_at_sample, [](const auto& it){return it.second > 1;});
        }
        if (play_long_note_ends) {
            for (const auto& it : long_note_ends) {
                notes_at_sample[it] = 1;
            }
        }
        copy_sample_at_points(
            sample,
            output_buffer,
            notes_at_sample,
            absolute_buffer_start
        );
    }

    data.samples = output_buffer.data();
    data.sampleCount = output_buffer.size();
    first_sample_of_next_buffer += output_buffer.size();

    return true;
};

void NoteClaps::onSeek(sf::Time timeOffset) {
    first_sample_of_next_buffer = music_time_to_samples(timeOffset);
    notes_at_sample.clear();
};
