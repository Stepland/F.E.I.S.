#pragma once

#include <set>

#include <json.hpp>

#include "special_numeric_types.hpp"

using Hakus = std::set<Fraction>;

nlohmann::ordered_json dump_hakus(const Hakus& hakus);