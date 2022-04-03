#pragma once

#include <optional>
#include <set>

#include <json.hpp>

#include "special_numeric_types.hpp"

using Hakus = std::set<Fraction>;

nlohmann::ordered_json dump_hakus(const Hakus& hakus);
std::optional<Hakus> load_hakus(const nlohmann::json& timing);