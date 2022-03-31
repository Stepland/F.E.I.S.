#pragma once

#include <json.hpp>

#include "special_numeric_types.hpp"

// Utilities for dumping to memon

bool is_expressible_as_240th(const Fraction& beat);
nlohmann::ordered_json beat_to_best_form(const Fraction& beat);
nlohmann::ordered_json beat_to_fraction_tuple(const Fraction& beat);