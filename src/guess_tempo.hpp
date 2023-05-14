#pragma once

#include <SFML/System/Time.hpp>
#include <filesystem>
#include <vector>

#include "special_numeric_types.hpp"
#include "utf8_sfml_redefinitions.hpp"

struct TempoEstimate {
    Decimal bpm;
    sf::Time offset;
    float fitness;
};

float evidence(const std::vector<std::size_t>& histogram, const std::size_t sample);
float confidence(const std::vector<std::size_t>& histogram, const std::size_t onset_position, const std::size_t interval);

std::vector<TempoEstimate> guess_tempo(const std::filesystem::path& audio);
std::set<std::size_t> detect_onsets(feis::InputSoundFile& music);
