#include "guess_tempo.hpp"
#include <SFML/Config.hpp>
#include <SFML/System/Utf.hpp>
#include <deque>

#include "aubio_cpp.hpp"
#include "custom_sfml_audio/precise_sound_stream.hpp"

std::vector<TempoEstimate> guess_tempo(feis::InputSoundFile& music) {
    const auto onsets = detect_onsets(music);
}

std::vector<sf::Time> detect_onsets(feis::InputSoundFile& music) {
    auto samplerate = music.getSampleRate();
    std::size_t win_s = 1024; // window size
    std::size_t hop_size = win_s / 4;
    std::size_t read = 0;
    std::vector<sf::Int16> samples(hop_size); // input audio buffer
    std::vector<sf::Time> onsets;
    auto detector = aubio::onset_detector("default", win_s, hop_size, samplerate);
    do {
        read = music.read(samples.data(), hop_size);
        auto onset = detector.detect(samples);
        // do something with the onsets
        if (onset) {
            onsets.push_back(samples_to_time(*onset, music.getSampleRate(), music.getChannelCount()));
        }
    } while ( read == hop_size );
    aubio_cleanup();
    return onsets;
}