#include "note_claps.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Time.hpp>
#include <limits>
#include <memory>
#include <stdexcept>

#include "../better_note.hpp"
#include "src/custom_sfml_audio/fake_pitched_sound_stream.hpp"
#include "src/custom_sfml_audio/sampler_callback.hpp"

NoteClaps::NoteClaps(
    const better::Notes* notes_,
    const better::Timing* timing_,
    const std::filesystem::path& assets,
    float pitch_
) :
    FakePitchedSoundStream(assets / "sounds" / "note.wav", pitch_),
    notes(notes_),
    timing(timing_)
{}

NoteClaps::NoteClaps(
    const better::Notes* notes_,
    const better::Timing* timing_,
    std::shared_ptr<sf::SoundBuffer> note_clap,
    float pitch
) :
    FakePitchedSoundStream(note_clap, pitch),
    notes(notes_),
    timing(timing_)
{}

void NoteClaps::set_notes_and_timing(const better::Notes* notes_, const better::Timing* timing_) {
    notes = notes_;
    timing = timing_;
}

std::shared_ptr<NoteClaps> NoteClaps::with_pitch(float pitch) {
    return std::make_shared<NoteClaps>(
        notes,
        timing,
        sample,
        pitch
    );
}

bool NoteClaps::onGetData(sf::SoundStream::Chunk& data) {
    if (timing != nullptr and notes != nullptr) {
        const auto absolute_buffer_start = first_sample_of_next_buffer;
        const std::int64_t absolute_buffer_end = first_sample_of_next_buffer + static_cast<std::int64_t>(output_buffer.size());
        const auto start_time = samples_to_music_time(absolute_buffer_start);
        const auto end_time = samples_to_music_time(absolute_buffer_end);
        const auto start_beat = timing->beats_at(start_time);
        const auto end_beat = timing->beats_at(end_time);

        notes->in(start_beat, end_beat, [&](const better::Notes::const_iterator& it){
            const auto beat = it->second.get_time();
            // ignore long notes that started before the current buffer
            if (beat < start_beat) {
                return;
            }
            const auto time = timing->time_at(beat);
            const auto sample = static_cast<std::int64_t>(music_time_to_samples(time));
            // interval_tree::in is inclusive of the upper bound but here we
            // don't want claps that *start* at the end sample since
            // it's an *exculsive* end
            if (sample < absolute_buffer_end) {
                notes_at_sample.insert(sample);
            }
        });
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
