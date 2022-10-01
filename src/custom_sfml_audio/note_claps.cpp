#include "note_claps.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Time.hpp>
#include <limits>
#include <memory>
#include <stdexcept>

#include "../better_note.hpp"

NoteClaps::NoteClaps(
    const better::Notes* notes_,
    const better::Timing* timing_,
    const std::filesystem::path& assets,
    float pitch_
) :
    pitch(pitch_),
    notes(notes_),
    timing(timing_),
    note_clap(std::make_shared<sf::SoundBuffer>())
{
    if (not note_clap->loadFromFile(assets / "sounds" / "note.wav")) {
        throw std::runtime_error("Could not load note clap audio file");
    }
    sf::SoundStream::initialize(note_clap->getChannelCount(), note_clap->getSampleRate());
    samples.resize(openAL_time_to_samples(sf::seconds(1)), 0);
}

NoteClaps::NoteClaps(
    const better::Notes* notes_,
    const better::Timing* timing_,
    std::shared_ptr<sf::SoundBuffer> note_clap_,
    float pitch_
) :
    pitch(pitch_),
    notes(notes_),
    timing(timing_),
    note_clap(note_clap_)
{
    sf::SoundStream::initialize(note_clap->getChannelCount(), note_clap->getSampleRate());
    samples.resize(openAL_time_to_samples(sf::seconds(1)), 0);
}

void NoteClaps::set_notes_and_timing(const better::Notes* notes_, const better::Timing* timing_) {
    notes = notes_;
    timing = timing_;
}

std::shared_ptr<NoteClaps> NoteClaps::with_pitch(float pitch) {
    return std::make_shared<NoteClaps>(
        notes,
        timing,
        note_clap,
        pitch
    );
}

bool NoteClaps::onGetData(sf::SoundStream::Chunk& data) {
    samples.assign(samples.size(), 0);
    if (timing != nullptr and notes != nullptr) {
        const auto absolute_buffer_start = current_sample;
        const std::int64_t absolute_buffer_end = current_sample + static_cast<std::int64_t>(samples.size());
        const auto start_time = samples_to_music_time(absolute_buffer_start);
        const auto end_time = samples_to_music_time(absolute_buffer_end);
        const auto start_beat = timing->beats_at(start_time);
        const auto end_beat = timing->beats_at(end_time);

        notes->in(start_beat, end_beat, [&](const better::Notes::const_iterator& it){
            const auto beat = it->second.get_time();
            const auto time = timing->time_at(beat);
            const auto sample = static_cast<std::int64_t>(music_time_to_samples(time));
            notes_at_sample[sample] += 1;
        });

        for (auto it = notes_at_sample.begin(); it != notes_at_sample.end();) {
            const std::int64_t absolute_clap_start = it->first;
            const std::int64_t absolute_clap_end = absolute_clap_start + static_cast<std::int64_t>(note_clap->getSampleCount());
            const std::int64_t absolute_clap_slice_start = std::max(
                absolute_clap_start,
                absolute_buffer_start
            );
            const std::int64_t absolute_clap_slice_end = std::min({
                absolute_clap_end,
                absolute_buffer_end,
                [&](const auto& it){
                    const auto next = std::next(it);
                    if (next != notes_at_sample.end()) {
                        return next->first;
                    } else {
                        return std::numeric_limits<std::int64_t>::max();
                    }
                }(it)
            });
            const std::int64_t slice_size = absolute_clap_slice_end - absolute_clap_slice_start;
            const std::int64_t slice_start_relative_to_clap_start = absolute_clap_slice_start - absolute_clap_start;
            const std::int64_t slice_start_relative_to_buffer_start = absolute_clap_slice_start - absolute_buffer_start;
            const auto input_start = note_clap->getSamples() + slice_start_relative_to_clap_start;
            const auto input_end = input_start + slice_size;
            const auto output_start = samples.begin() + slice_start_relative_to_buffer_start;
            // this code is SURPRISINGLY hard to get right for how little it
            // seems to be doing.
            // the slice size SHOULD always be positive but if for whatever
            // reason the code is still wrong and the computation returns
            // a bogus value we just give up playing the samples instead of
            // risking a segfault
            if (slice_size > 0) {
                std::copy(
                    input_start,
                    input_end,
                    output_start
                );
            }
            if (absolute_clap_end <= absolute_buffer_end) {
                it = notes_at_sample.erase(it);
            } else {
                ++it;
            }
        }
    }

    data.samples = samples.data();
    data.sampleCount = samples.size();
    current_sample += samples.size();

    return true;
};

void NoteClaps::onSeek(sf::Time timeOffset) {
    current_sample = music_time_to_samples(timeOffset);
    notes_at_sample.clear();
};

std::int64_t NoteClaps::openAL_time_to_samples(sf::Time position) const {
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep Int64 as the data type throughout the whole operation.
    return ((static_cast<std::int64_t>(position.asMicroseconds()) * note_clap->getSampleRate() * note_clap->getChannelCount()) + 500000) / 1000000;
}

sf::Time NoteClaps::samples_to_openAL_time(std::int64_t samples) const {
    sf::Time position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (note_clap->getSampleRate() != 0 && note_clap->getChannelCount() != 0)
        position = sf::microseconds((samples * 1000000) / (note_clap->getChannelCount() * note_clap->getSampleRate()));

    return position;
}

std::int64_t NoteClaps::music_time_to_samples(sf::Time position) const {
    return openAL_time_to_samples(position / pitch);
}

sf::Time NoteClaps::samples_to_music_time(std::int64_t samples) const {
    return samples_to_openAL_time(samples) * pitch;
}