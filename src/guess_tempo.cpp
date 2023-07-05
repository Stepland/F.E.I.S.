#include "guess_tempo.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <cassert>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <numbers>
#include <numeric>
#include <ranges>
#include <thread>

#include <SFML/Config.hpp>
#include <SFML/System/Utf.hpp>
#include <eigen_polyfit.hpp>
#include <vector>

#include "Eigen/src/Core/Array.h"
#include "aubio_cpp.hpp"
#include "special_numeric_types.hpp"

std::vector<TempoCandidate> guess_tempo(const std::filesystem::path& audio) {
    feis::InputSoundFile music;
    music.open_from_path(audio);
    const auto onsets = detect_onsets(music);
    const auto bpm_candidates = estimate_bpm(onsets, music.getSampleRate());
    return estimate_offset(bpm_candidates, music);
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


std::vector<BPMFitness> estimate_bpm(const std::set<std::size_t>& onsets, const std::size_t sample_rate) {
    const auto broad_fitness = broad_interval_test(onsets, sample_rate);
    const auto corrected_fitness = correct_bias(broad_fitness);
    auto interval_candidates = narrow_interval_test(broad_fitness, corrected_fitness, onsets, sample_rate);
    return select_bpm_candidates(interval_candidates, onsets, sample_rate);
}

const float min_tested_bpm = 89.f;
const float max_tested_bpm = 205.f;
const std::size_t hamming_window_size = 1024;

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
                    fitness_of_interval_range,
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

std::vector<IntervalFitness> fitness_of_interval_range(const std::set<std::size_t>& onsets, const std::size_t start_interval, const std::size_t count, const std::size_t stride) {
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
        Eigen::Index max_index;
        const auto fitness = confidence.maxCoeff(&max_index);
        fitness_values.push_back({tested_interval, fitness, static_cast<std::size_t>(max_index)});
    }
    return fitness_values;
}

#ifndef EIGEN_VECTORIZE
    #error eigen vectorization is off !
#endif

Eigen::ArrayXf load_hamming_coefficients() {
    Eigen::ArrayXf n = Eigen::ArrayXf::LinSpaced(hamming_window_size, 0, hamming_window_size - 1);
    return 0.54f - 0.46f * (n * ((2.f * std::numbers::pi_v<float>) / hamming_window_size - 1.f)).cos();
}

const Eigen::ArrayXf hamming_coefficients = load_hamming_coefficients();

float evidence(const Eigen::ArrayXf& histogram, const std::size_t sample) {
    const std::int64_t start_index = static_cast<std::int64_t>(sample) - static_cast<std::int64_t>(hamming_window_size) / 2;
    const std::int64_t end_index = start_index + static_cast<std::int64_t>(hamming_window_size); // exclusive
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

std::vector<IntervalFitness> narrow_interval_test(
    const std::vector<IntervalFitness>& broad_fitness,
    const Eigen::ArrayXf& corrected_broad_fitness,
    const std::set<std::size_t>& onsets,
    const std::size_t sample_rate
) {
    const auto max_fitness = corrected_broad_fitness.maxCoeff();
    std::vector<std::future<std::vector<IntervalFitness>>> futures;
    for (std::size_t i = 0; i < corrected_broad_fitness.size(); i++) {
        if ((corrected_broad_fitness[i] / max_fitness) > 0.4f) {
            const auto interval = broad_fitness.at(i).interval;
            futures.emplace_back(
            std::async(
                    std::launch::async,
                    std::bind(fitness_of_interval_range, std::cref(onsets), interval - 9, 19, 1)
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
    return candidates;
}

std::vector<BPMFitness> select_bpm_candidates(
    std::vector<IntervalFitness>& interval_candidates,
    const std::set<std::size_t>& onsets,
    const std::size_t sample_rate
) {
    // Bram van de Wetering's original paper [BvdW] :
    // "BPM candidates that differ by less than 0.1 from a multiple of a
    // candidate with a higher fitness value are rejected."
    std::sort(
        interval_candidates.begin(),
        interval_candidates.end(),
        [](const IntervalFitness& a, const IntervalFitness& b){return a.fitness < b.fitness;}
    );
    std::vector<IntervalFitness> deduped_candidates;
    while (not interval_candidates.empty()) {
        deduped_candidates.push_back(interval_candidates.back());
        interval_candidates.pop_back();
        const auto reference_interval = deduped_candidates.back().interval;
        const Fraction reference_bpm = Fraction{60 * sample_rate} / Fraction{reference_interval};
        std::erase_if(interval_candidates, [&](const IntervalFitness& f){
            const Fraction potential_multiple = round_fraction(Fraction(reference_interval, f.interval));
            const Fraction candidate_bpm = Fraction{60 * sample_rate} / Fraction{f.interval};
            Fraction diff = (reference_bpm * potential_multiple) - candidate_bpm;
            if (diff < 0) {
                diff *= -1;
            } 
            return diff < 0.1f;
        });
    }
    // [BvdW] :
    // "BPM candidates that differ by less than 0.05 from an integer value are
    // rounded and re-evaluated with a (possibly fractional) interval. If the
    // fitness value of the rounded BPM exceeds 99% of the original fitness
    // value, the rounded BPM is assumed to be correct and replaces the original
    // BPM candidate."
    std::vector<BPMFitness> bpm_candidates;
    std::transform(
        deduped_candidates.begin(),
        deduped_candidates.end(),
        std::back_inserter(bpm_candidates),
        [&](const IntervalFitness& f){
            return BPMFitness{
                Fraction{60 * sample_rate} / Fraction{f.interval},
                f.fitness
            };
        }
    );
    for (auto& candidate: bpm_candidates) {
        const Fraction rounded_bpm = round_fraction(candidate.bpm);
        Fraction diff = candidate.bpm - rounded_bpm;
        if (diff < 0) {
            diff *= -1;
        }
        if (diff < 0.05f) {
            const auto rounded_bpm_fitness  = fitness_of_bpm(onsets, sample_rate, rounded_bpm);
            if (rounded_bpm_fitness.fitness > 0.99f * candidate.fitness) {
                candidate.bpm = rounded_bpm;
                candidate.fitness = rounded_bpm_fitness.fitness;
                candidate.max_onset = rounded_bpm_fitness.max_onset;
            }
        }
    }
    // At this point [BvdW] says :
    // 
    // "if the ratio between the fitness values of the primary and secondary BPM
    // candidates is less than 1.05, the fitness values of all BPM candidates
    // are recomputed with accurate, fractional intervals."
    //
    // But I just really don't get why ... we've already recomputed the fitness
    // of BPMs we changed at the previous step, and the other BPMs haven't
    // changed so there's no reason to recompute their fitness either ...
    // Instead I just sort the candidates by fitness and return them
    std::sort(
        bpm_candidates.begin(),
        bpm_candidates.end(),
        [](const BPMFitness& a, const BPMFitness& b){return a.fitness < b.fitness;}
    );
    return bpm_candidates;
}

Fitness fitness_of_bpm(
    const std::set<std::size_t>& onsets,
    const std::size_t sample_rate,
    const Fraction BPM
) {
    const Fraction fractional_interval = Fraction(60) * Fraction(sample_rate) / Fraction(BPM);
    const std::size_t rounded_interval = static_cast<std::size_t>(floor_fraction(fractional_interval)) + 1;
    Eigen::ArrayXf evidence_cache(rounded_interval);
    Eigen::ArrayXf confidence(rounded_interval);
    Eigen::ArrayX<std::size_t> int_histogram = Eigen::ArrayX<std::size_t>::Zero(rounded_interval);
    Eigen::ArrayXf histogram;
    std::ranges::for_each(onsets, [&](std::size_t s){
        const auto fractional_bin = Fraction{s} % fractional_interval;
        const auto bin = static_cast<std::size_t>(floor_fraction(fractional_bin));
        int_histogram[bin] += 1;
    });
    histogram = int_histogram.cast<float>();
    for (std::size_t onset = 0; onset < rounded_interval; onset++) {
        evidence_cache[onset] = evidence(histogram, onset);
    }
    confidence = evidence_cache;
    const auto first_half_interval = rounded_interval / 2;
    const auto second_half_interval = rounded_interval - first_half_interval;
    confidence.head(first_half_interval) += evidence_cache.tail(first_half_interval) / 2;
    confidence.tail(second_half_interval) += evidence_cache.head(second_half_interval) / 2;
    Eigen::Index max_index;
    const auto fitness = confidence.maxCoeff(&max_index);
    return {fitness, static_cast<std::size_t>(max_index)};
}

std::vector<TempoCandidate> estimate_offset(const std::vector<BPMFitness>& bpm_candidates, feis::InputSoundFile& music) {
    std::vector<TempoCandidate> result;
    result.reserve(bpm_candidates.size());
    music.seek(0);
    const std::size_t mono_samples = music.getSampleCount() / music.getChannelCount();
    std::vector<float> amplitude(mono_samples);
    const std::size_t chunk_size = 4096 * music.getChannelCount();
    std::size_t read;
    std::vector<sf::Int16> buffer(chunk_size);
    do {
        read = music.read(buffer.data(), chunk_size);
        for (std::size_t i = 0; i < read / music.getChannelCount(); i++) {
            float downmixed_sample = 0;
            for (std::size_t channel = 0; channel < music.getChannelCount(); channel++) {
                downmixed_sample += buffer[i * music.getChannelCount() + channel];
            }
            amplitude[i] = downmixed_sample / channel_count;
        }
    } while ( read == chunk_size );

    for (const auto& bpm_candidate : bpm_candidates) {
        const Fraction initial_offset_estimate = Fraction{bpm_candidate.max_onset, music.getSampleRate()};
    }
}