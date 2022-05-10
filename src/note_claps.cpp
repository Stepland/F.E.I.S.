#include "note_claps.hpp"

NoteClaps::NoteClaps(const std::filesystem::path& assets) {
    const auto path = assets / "sounds" / "note.wav";
    if (not clap.loadFromFile(path)) {
        throw std::runtime_error("Could not open "+path.string());
    }
    output_buffer.resize(clap.getSampleRate()*clap.getChannelCount(), 0);
    initialize(clap.getChannelCount(), clap.getSampleRate());
    initialize_open_al_extension();
}

void NoteClaps::set_notes_and_timing(const better::Notes* notes_, const better::Timing* timing_) {
    notes = notes_;
    timing = timing_;
}

bool NoteClaps::onGetData(sf::SoundStream::Chunk& data) {
    output_buffer.assign(output_buffer.size(), 0);
    if (timing != nullptr and notes != nullptr) {
        const auto start_sample = current_sample;
        const auto end_sample = current_sample + static_cast<std::int64_t>(output_buffer.size());
        const auto start_time = samplesToTime(start_sample);
        const auto end_time = samplesToTime(end_sample);
        const auto start_beat = timing->beats_at(start_time);
        const auto end_beat = timing->beats_at(end_time);

        notes->in(start_beat, end_beat, [&](const better::Notes::const_iterator& it){
            const auto beat = it->second.get_time();
            const auto time = timing->time_at(beat);
            const auto sample = static_cast<std::int64_t>(timeToSamples(time));
            notes_at_sample[sample] += 1;
        });

        for (auto it = notes_at_sample.begin(); it != notes_at_sample.end();) {
            // Should we still be playing the clap ?
            const auto next = std::next(it);
            const auto last_audible_start = start_sample - static_cast<std::int64_t>(clap.getSampleCount());
            if (it->first <= last_audible_start) {
                it = notes_at_sample.erase(it);
            } else {
                const auto full_clap_start_in_buffer = static_cast<std::int64_t>(it->first) - static_cast<std::int64_t>(start_sample);
                const auto slice_start_in_buffer = std::max(std::int64_t(0), full_clap_start_in_buffer);
                const auto full_clap_end_in_buffer = full_clap_start_in_buffer + static_cast<std::int64_t>(clap.getSampleCount());
                auto slice_end_in_buffer = full_clap_end_in_buffer;
                bool clap_finished_playing_in_current_buffer = true;
                if (next != notes_at_sample.end()) {
                    slice_end_in_buffer = std::min(
                        slice_end_in_buffer,
                        static_cast<std::int64_t>(next->first) - static_cast<std::int64_t>(start_sample)
                    );
                } else if (slice_end_in_buffer > static_cast<std::int64_t>(output_buffer.size())) {
                    clap_finished_playing_in_current_buffer = false;
                    slice_end_in_buffer = static_cast<std::int64_t>(output_buffer.size());
                }
                auto slice_start_in_clap = slice_start_in_buffer - full_clap_start_in_buffer;
                auto slice_size = std::min(
                    slice_end_in_buffer - slice_start_in_buffer,
                    static_cast<std::int64_t>(clap.getSampleCount()) - slice_start_in_clap
                );
                for (std::int64_t i = 0; i < slice_size; i++) {
                    output_buffer[slice_start_in_buffer + i] = clap.getSamples()[slice_start_in_clap + i];
                }
                if (clap_finished_playing_in_current_buffer) {
                    it = notes_at_sample.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    data.samples = output_buffer.data();
    data.sampleCount = output_buffer.size();
    current_sample += output_buffer.size();

    return true;
}

void NoteClaps::onSeek(sf::Time timeOffset) {
    current_sample = timeToSamples(timeOffset);
    notes_at_sample.clear();
}

std::int64_t NoteClaps::timeToSamples(sf::Time position) const
{
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep Int64 as the data type throughout the whole operation.
    return ((static_cast<std::int64_t>(position.asMicroseconds()) * getSampleRate() * getChannelCount()) + 500000) / 1000000;
}


////////////////////////////////////////////////////////////
sf::Time NoteClaps::samplesToTime(std::int64_t samples) const
{
    sf::Time position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (clap.getSampleRate() != 0 && clap.getChannelCount() != 0)
        position = sf::microseconds(static_cast<std::int64_t>((samples * 1000000) / (getChannelCount() * getSampleRate())));

    return position;
}