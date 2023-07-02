#include "guess_tempo.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <cassert>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <numbers>
#include <numeric>
#include <ranges>
#include <thread>

#include <SFML/Config.hpp>
#include <SFML/System/Utf.hpp>
#include <eigen_polyfit.hpp>

#include "Eigen/src/Core/Array.h"
#include "aubio_cpp.hpp"

std::vector<float> guess_tempo(const std::filesystem::path& path) {
    feis::InputSoundFile music;
    music.open_from_path(path);
    const auto onsets = detect_onsets(music);
    estimate_bpm(onsets, music.getSampleRate());
    // estimate offsets
}

void estimate_bpm(const std::set<std::size_t>& onsets, const std::size_t sample_rate) {
    const auto fitness = broad_interval_test(onsets, sample_rate);
    const auto corrected_fitness = correct_bias(fitness);
    const auto max_fitness = corrected_fitness.maxCoeff();
    std::vector<std::future<std::vector<IntervalFitness>>> futures;
    for (std::size_t i = 0; i < corrected_fitness.size(); i++) {
        if ((corrected_fitness[i] / max_fitness) > 0.4f) {
            const auto interval = fitness.at(i).interval;
            futures.emplace_back(
            std::async(
                    std::launch::async,
                    std::bind(test_intervals, std::cref(onsets), interval - 9, 19, 1)
                )
            );
        }
    }
    std::vector<IntervalFitness> candidates;
    for (auto& future : futures) {
        const auto results = future.get();
        const auto it = std::ranges::max_element(results, {}, [](const IntervalFitness& f){return f.fitness;});
        candidates.push_back(*it);
    }
    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const IntervalFitness& a, const IntervalFitness& b){return a.fitness > b.fitness;}
    );
    std::vector<IntervalFitness>
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

const float min_tested_bpm = 89.f;
const float max_tested_bpm = 205.f;
const std::size_t hamming_window_size = 1024;

#ifndef EIGEN_VECTORIZE
    #error eigen vectorization is off !
#endif

Eigen::ArrayXf load_hamming_coefficients() {
    Eigen::ArrayXf n = Eigen::ArrayXf::LinSpaced(hamming_window_size, 0, hamming_window_size - 1);
    return 0.54f - 0.46f * (n * ((2.f * std::numbers::pi_v<float>) / hamming_window_size - 1.f)).cos();
}

const Eigen::ArrayXf hamming_coefficients = load_hamming_coefficients();

float evidence(const Eigen::ArrayXf& histogram, const std::size_t sample) {
    const std::int64_t start_index = static_cast<std::int64_t>(sample) - hamming_window_size / 2;
    const std::int64_t end_index = start_index + hamming_window_size; // exclusive
    if (start_index < 0) {
        const auto elements_before_first = std::abs(start_index);
        return (
            (
                hamming_coefficients.head(elements_before_first)
                * histogram.tail(elements_before_first)
            ).sum()
            + (
                hamming_coefficients.tail(end_index)
                * histogram.head(end_index)
            ).sum()
        );
    } else if (end_index > histogram.size()) {
        const auto elements_after_last = end_index - histogram.size();
        const auto elements_before_last = histogram.size() - start_index;
        return (
            (
                hamming_coefficients.tail(elements_after_last)
                * histogram.head(elements_after_last)
            ).sum()
            + (
                hamming_coefficients.head(elements_before_last)
                * histogram.tail(elements_before_last)
            ).sum()
        );
    } else {
        return (hamming_coefficients * histogram.segment(start_index, hamming_window_size)).sum();
    }
}

std::vector<IntervalFitness> broad_interval_test(const std::set<std::size_t>& onsets, const std::size_t sample_rate) {
    const std::size_t min_tested_interval = sample_rate * 60.f / max_tested_bpm;
    const std::size_t max_tested_interval = sample_rate * 60.f / min_tested_bpm;
    const std::size_t interval_range_width = max_tested_interval - min_tested_interval + 1;
    const std::size_t stride = 10;
    const std::size_t number_of_tested_intervals = (interval_range_width - 1) / stride + 1;
    std::vector<IntervalFitness> fitness_values;
    fitness_values.reserve(number_of_tested_intervals);
    const auto threads = std::thread::hardware_concurrency();
    const auto chunk_size = number_of_tested_intervals / threads;
    std::vector<std::future<std::vector<IntervalFitness>>> futures;
    for (std::size_t thread_index = 0; thread_index < threads; thread_index++) {
        futures.emplace_back(
            std::async(
                std::launch::async,
                std::bind(
                    test_intervals,
                    std::cref(onsets),
                    min_tested_interval + thread_index * chunk_size * stride,
                    chunk_size,
                    stride
                )
            )
        );
    }
    for (auto& future : futures) {
        const auto new_values = future.get();
        fitness_values.insert(fitness_values.end(), new_values.begin(), new_values.end());
    }
    return fitness_values;
}

std::vector<IntervalFitness> test_intervals(const std::set<std::size_t>& onsets, const std::size_t start_interval, const std::size_t count, const std::size_t stride) {
    std::vector<IntervalFitness> fitness_values;
    fitness_values.reserve(count);
    const auto max_tested_interval = start_interval + (count - 1) * stride;
    Eigen::ArrayXf evidence_cache(max_tested_interval);
    Eigen::ArrayXf confidence(max_tested_interval);
    Eigen::ArrayX<std::size_t> int_histogram(max_tested_interval);
    Eigen::ArrayXf histogram;
    for (std::size_t interval_index = 0; interval_index < count; interval_index++) {
        const auto tested_interval = start_interval + interval_index * stride;
        int_histogram.setZero(tested_interval);
        std::ranges::for_each(onsets, [&](std::size_t s){
            int_histogram[s % tested_interval] += 1;
        });
        histogram = int_histogram.cast<float>();
        evidence_cache.resize(tested_interval);
        for (std::size_t onset = 0; onset < tested_interval; onset++) {
            evidence_cache[onset] = evidence(histogram, onset);
        }
        confidence.resize(tested_interval);
        confidence = evidence_cache;
        const auto first_half_interval = tested_interval / 2;
        const auto second_half_interval = tested_interval - first_half_interval;
        confidence.head(first_half_interval) += evidence_cache.tail(first_half_interval) / 2;
        confidence.tail(second_half_interval) += evidence_cache.head(second_half_interval) / 2;
        fitness_values.push_back({tested_interval, confidence.maxCoeff()});
    }
    return fitness_values;
}

Eigen::ArrayXf correct_bias(const std::vector<IntervalFitness>& fitness_results) {
    std::vector<float> y_values(fitness_results.size());
    std::transform(
        fitness_results.cbegin(),
        fitness_results.cend(),
        y_values.begin(),
        [](const IntervalFitness& f){return f.fitness;}
    );
    const auto coeffs = polyfit(y_values, 3);
    Eigen::ArrayXf fitness(y_values.size());
    std::copy(y_values.cbegin(), y_values.cend(), fitness.begin());
    return fitness - (
        coeffs[0]
        + coeffs[1] * fitness
        + coeffs[2] * fitness.pow(2)
        + coeffs[3] * fitness.pow(3)
    );
}