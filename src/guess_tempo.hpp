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

std::vector<TempoEstimate> guess_tempo(feis::InputSoundFile& music);
std::vector<sf::Time> detect_onsets(feis::InputSoundFile& music);
