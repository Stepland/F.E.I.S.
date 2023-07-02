#pragma once

#include <filesystem>
#include <vector>
#include <set>

#include <Eigen/Dense>
#include <SFML/System/Time.hpp>

#include "Eigen/src/Core/Matrix.h"
#include "utf8_sfml_redefinitions.hpp"


struct IntervalFitness {
    std::size_t interval;
    float fitness;
};

std::vector<float> guess_tempo(const std::filesystem::path& audio);

std::set<std::size_t> detect_onsets(feis::InputSoundFile& music);

void estimate_bpm(const std::set<std::size_t>& onsets, const std::size_t sample_rate);
std::vector<IntervalFitness> broad_interval_test(const std::set<std::size_t>& onsets, const std::size_t sample_rate);
std::vector<IntervalFitness> test_intervals(
    const std::set<std::size_t>& onsets,
    const std::size_t start_interval,
    const std::size_t count,
    const std::size_t stride
);
Eigen::ArrayXf correct_bias(const std::vector<IntervalFitness>& fitness_results);
