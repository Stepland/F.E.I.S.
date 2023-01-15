#pragma once

#include <atomic>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <thread>

#include <imgui/imgui.h>
#include <SFML/System/Time.hpp>
#include <vector>

#include "../custom_sfml_audio/open_music.hpp"
#include "utf8_sfml.hpp"

struct DataPoint {
    using value_type = std::int16_t;
    value_type min = 0;
    value_type max = 0;
};

using DataFrame = std::vector<DataPoint>;
using Channels = std::vector<DataFrame>;

class WaveformView {
public:
    explicit WaveformView(const std::filesystem::path& file);
    void draw(const sf::Time current_time);
    void draw_settings();
    bool display = false;
private:
    feis::HoldFileStreamMixin<sf::InputSoundFile> sound_file;
    std::atomic<bool> data_is_ready = false;
    std::map<unsigned int, Channels> channels_per_chunk_size;
    std::optional<std::map<unsigned int, Channels>::iterator> selected_size;
    std::jthread worker;

    void prepare_data();
};