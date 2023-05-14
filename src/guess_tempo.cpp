#include "guess_tempo.hpp"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <limits>
#include <numbers>

#include <SFML/Config.hpp>
#include <SFML/System/Utf.hpp>
#include <numeric>
#include <ranges>

#include "aubio_cpp.hpp"
#include "custom_sfml_audio/precise_sound_stream.hpp"

const float min_tested_bpm = 89.f;
const float max_tested_bpm = 205.f;
const std::size_t hamming_window_size = 1024;

consteval float hamming_coefficient(std::size_t index, std::size_t window_size) {
    return 0.54f - 0.46f * std::cos(2* std::numbers::pi_v<float> * index / window_size - 1);
}

consteval std::array<float, hamming_window_size> load_hamming_coefficients() {
    std::array<float, hamming_window_size> result;
    for (std::size_t i = 0; i < hamming_window_size; i++) {
        result[i] = hamming_coefficient(i, hamming_window_size);
    }
    return result;
}

const auto hamming_coefficients = load_hamming_coefficients();

float evidence(const std::vector<std::size_t>& histogram, const std::size_t sample) {
    float result;
    for (std::size_t hamming_index = 0; hamming_index < hamming_coefficients.size(); hamming_index++) {
        const auto histogram_index = (sample - hamming_window_size / 2 + hamming_index) % histogram.size();
        result += hamming_coefficients[hamming_index] * histogram[histogram_index];
    }
    return result;
}

float confidence(const std::vector<std::size_t>& histogram, const std::size_t onset_position, const std::size_t interval) {
    return evidence(histogram, onset_position) + evidence(histogram, onset_position + interval / 2) / 2;
}

std::vector<TempoEstimate> guess_tempo(const std::filesystem::path& path) {
    feis::InputSoundFile music;
    music.open_from_path(path);
    const auto onsets = detect_onsets(music);
    const std::size_t min_tested_interval = music.getSampleRate() * 60.f / max_tested_bpm;
    const std::size_t max_tested_interval = music.getSampleRate() * 60.f / min_tested_bpm;
    const std::size_t interval_range_width = max_tested_interval - min_tested_interval + 1;
    const std::size_t number_of_tested_intervals = (interval_range_width - 1) / 10 + 1;
    std::vector<float> fitness_values;
    fitness_values.reserve(number_of_tested_intervals);
    for (std::size_t tested_interval = min_tested_interval; tested_interval <= max_tested_interval; tested_interval += 10) {
        std::vector<std::size_t> histogram(tested_interval);
        std::ranges::for_each(onsets, [&](std::size_t s){
            histogram[s % tested_interval] += 1;
        });
        float max_confidence = -std::numeric_limits<float>::infinity();
        for (std::size_t onset = 0; onset < histogram.size(); onset++) {
            const float new_confidence = confidence(histogram, onset, tested_interval);
            max_confidence = std::max(max_confidence, new_confidence);
        }
        fitness_values.push_back(max_confidence);
    }
    
}

std::set<std::size_t> detect_onsets(feis::InputSoundFile& music) {
    const auto sample_rate = music.getSampleRate();
    const auto channel_count = music.getChannelCount();
    std::size_t hop_size = 256;
    std::size_t window_size = 1024;
    std::size_t read = 0;
    std::size_t chunk_size = hop_size * music.getChannelCount();
    std::vector<sf::Int16> samples(chunk_size);
    std::vector<sf::Int16> downmixed_samples(hop_size);
    std::set<std::size_t> onsets;
    auto detector = aubio::onset_detector("specflux", window_size, hop_size, sample_rate);
    do {
        read = music.read(samples.data(), chunk_size);
        std::ranges::fill(downmixed_samples, 0);
        for (std::size_t i = 0; i < read / channel_count; i++) {
            std::int64_t downmixed_sample = 0;
            for (std::size_t channel = 0; channel < channel_count; channel++) {
                downmixed_sample += samples[i*channel_count+channel];
            }
            downmixed_samples[i] = downmixed_sample / channel_count;
        }
        auto onset = detector.detect(downmixed_samples);
        if (onset) {
            onsets.emplace(*onset);
        }
    } while ( read == chunk_size );
    aubio_cleanup();
    return onsets;
}