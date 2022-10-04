#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Config.hpp>

void copy_sample_at_points(
    const std::shared_ptr<sf::SoundBuffer>& sample,
    std::span<sf::Int16> output_buffer,
    std::set<std::int64_t>& starting_points,
    std::int64_t absolute_buffer_start
);