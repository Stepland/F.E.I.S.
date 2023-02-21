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
#include "../config.hpp"
#include "../utf8_sfml.hpp"

struct DataPoint {
    using value_type = std::int16_t;
    value_type min = 0;
    value_type max = 0;
};

using DataFrame = std::vector<DataPoint>;
using Channels = std::vector<DataFrame>;

class WaveformView {
public:
    explicit WaveformView(const std::filesystem::path& audio, config::Config& config);
    void draw(const sf::Time current_time);
    void draw_settings();
    bool display = false;

    void set_zoom(int zoom);
    void zoom_in();
    void zoom_out();
private:
    feis::HoldFileStreamMixin<sf::InputSoundFile> sound_file;
    std::atomic<bool> data_is_ready = false;
    std::vector<std::pair<unsigned int, Channels>> channels_per_chunk_size;
    std::jthread worker;

    int& zoom;

    void prepare_data();
};

Channels load_initial_summary(
    feis::HoldFileStreamMixin<sf::InputSoundFile>& sound_file,
    const unsigned int window_size
);
Channels downsample_to_half(const Channels& summary);