#pragma once

#include <filesystem>
#include <vector>
#include <set>

#include <Eigen/Dense>
#include <SFML/System/Time.hpp>

#include "utf8_sfml_redefinitions.hpp"

/*
struct TempoEstimate {
    Decimal bpm;
    sf::Time offset;
    float fitness;
};
*/

std::vector<float> guess_tempo(const std::filesystem::path& audio);
std::set<std::size_t> detect_onsets(feis::InputSoundFile& music);
std::vector<float> compute_fitness(const std::set<std::size_t>& onsets, const std::size_t sample_rate);
std::vector<float> compute_fitness_at_intervals(
    const std::set<std::size_t>& onsets,
    const std::size_t start_interval,
    const std::size_t count,
    const std::size_t stride
);
