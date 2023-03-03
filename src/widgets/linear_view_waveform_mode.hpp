#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

#include <SFML/Audio/InputSoundFile.hpp>

#include "../utf8_sfml.hpp"
#include "cache.hpp"

namespace linear_view::mode::waveform {
    struct DataPoint {
        using value_type = std::int16_t;
        value_type min = 0;
        value_type max = 0;
    };

    using DataFrame = std::vector<DataPoint>;
    using Channels = std::vector<DataFrame>;

    Channels load_initial_summary(
        feis::HoldFileStreamMixin<sf::InputSoundFile>& sound_file,
        const unsigned int window_size
    );
    Channels downsample_to_half(const Channels& summary);

    Toolkit::Cache<std::filesystem::path, Channels> waveform_cache;
}