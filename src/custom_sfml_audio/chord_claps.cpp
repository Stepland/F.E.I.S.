#include "chord_claps.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Time.hpp>
#include <limits>
#include <memory>
#include <stdexcept>

#include "../better_note.hpp"
#include "src/custom_sfml_audio/fake_pitched_sound_stream.hpp"
#include "src/custom_sfml_audio/sampler_callback.hpp"
#include "src/special_numeric_types.hpp"

ChordClaps::ChordClaps(
    const std::shared_ptr<better::Notes>& notes_,
    const std::shared_ptr<better::Timing>& timing_,
    const std::filesystem::path& assets,
    float pitch_
) :
    FakePitchedSoundStream(assets / "sounds" / "chord.wav", pitch_),
    notes(notes_),
    timing(timing_)
{}

ChordClaps::ChordClaps(
    const std::shared_ptr<better::Notes>& notes_,
    const std::shared_ptr<better::Timing>& timing_,
    std::shared_ptr<FakePitchedSoundStream::sound_buffer_type> note_clap,
    float pitch
) :
    FakePitchedSoundStream(note_clap, pitch),
    notes(notes_),
    timing(timing_)
{}

std::shared_ptr<ChordClaps> ChordClaps::with_notes_and_timing(
    const std::shared_ptr<better::Notes>& notes_,
    const std::shared_ptr<better::Timing>& timing_
) {
    return std::make_shared<ChordClaps>(
        notes_,
        timing_,
        sample,
        pitch
    );
}

std::shared_ptr<ChordClaps> ChordClaps::with_pitch(float new_pitch) {
    return std::make_shared<ChordClaps>(
        notes,
        timing,
        sample,
        new_pitch
    );
}

bool ChordClaps::onGetData(sf::SoundStream::Chunk& data) {
    if (timing and notes) {
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

        notes->in(start_beat, end_beat, [&](const better::Notes::const_iterator& it){
            count_clap_at(it->second.get_time());
        });
        std::erase_if(notes_at_sample, [](const auto& it){return it.second <= 1;});
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

void ChordClaps::onSeek(sf::Time timeOffset) {
    first_sample_of_next_buffer = music_time_to_samples(timeOffset);
    notes_at_sample.clear();
};
