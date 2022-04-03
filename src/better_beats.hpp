#pragma once

#include <cstdint>

#include <json.hpp>

#include "special_numeric_types.hpp"

// Utilities for dumping to memon

bool is_expressible_as_240th(const Fraction& beat);
nlohmann::ordered_json beat_to_best_form(const Fraction& beat);
nlohmann::ordered_json beat_to_fraction_tuple(const Fraction& beat);

Fraction load_memon_1_0_0_beat(const nlohmann::json& json, std::uint64_t resolution);