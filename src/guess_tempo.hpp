#pragma once

#include <filesystem>
#include <vector>
#include <set>

#include <Eigen/Dense>
#include <SFML/System/Time.hpp>

#include "special_numeric_types.hpp"
#include "utf8_sfml_redefinitions.hpp"


struct IntervalFitness {
    std::size_t interval;
    float fitness;
};

struct BPMFitness {
    Fraction bpm;
    float fitness;
};

struct BPMCandidate {
    Fraction bpm;
    std::size_t offset;
    float fitness;
};

std::vector<BPMCandidate> guess_tempo(const std::filesystem::path& audio);

std::set<std::size_t> detect_onsets(feis::InputSoundFile& music);

void estimate_bpm(const std::set<std::size_t>& onsets, const std::size_t sample_rate);
std::vector<IntervalFitness> broad_interval_test(const std::set<std::size_t>& onsets, const std::size_t sample_rate);
std::vector<IntervalFitness> fitness_of_interval_range(
    const std::set<std::size_t>& onsets,
    const std::size_t start_interval,
    const std::size_t count,
    const std::size_t stride
);
float evidence(const Eigen::ArrayXf& histogram, const std::size_t sample);
Eigen::ArrayXf correct_bias(const std::vector<IntervalFitness>& fitness_results);
std::vector<IntervalFitness> narrow_interval_test(
    const std::vector<IntervalFitness>& broad_fitness,
    const Eigen::ArrayXf& corrected_fitness,
    const std::set<std::size_t>& onsets,
    const std::size_t sample_rate
);
std::vector<BPMFitness> select_bpm_candidates(
    std::vector<IntervalFitness>& interval_candidates,
    const std::size_t sample_rate,
    const std::set<std::size_t>& onsets
);
float fitness_of_bpm(const std::set<std::size_t>& onsets, const std::size_t sample_rate, const Fraction BPM);
