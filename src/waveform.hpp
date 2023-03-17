#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

#include <SFML/Audio/InputSoundFile.hpp>

#include "cache.hpp"
#include "utf8_sfml.hpp"

namespace waveform {
    struct DataPoint {
        using value_type = std::int16_t;
        value_type min = 0;
        value_type max = 0;
    };

    using DataFrame = std::vector<DataPoint>;
    using Channels = std::vector<DataFrame>;
    struct Waveform {
        std::map<unsigned int, Channels> channels_per_chunk_size;
        std::vector<unsigned int> chunk_sizes; // for fast nth chunk size access
        std::size_t sample_rate;
        std::size_t channel_count;
    };

    Channels load_initial_summary(
        feis::HoldFileStreamMixin<sf::InputSoundFile>& sound_file,
        const unsigned int window_size
    );
    Channels downsample_to_half(const Channels& summary);
    std::optional<Waveform> compute_waveform(const std::filesystem::path& audio);

    struct Cache : Toolkit::Cache<std::filesystem::path, std::optional<waveform::Waveform>> {
        Cache();
    };
}