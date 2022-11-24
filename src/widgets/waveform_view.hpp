#pragma once

#include <memory>
#include <thread>

#include <imgui/imgui.h>
#include <SFML/System/Time.hpp>

#include "../custom_sfml_audio/open_music.hpp"

class WaveformView {
public:
    explicit WaveformView(const std::shared_ptr<OpenMusic>& audio);
    void draw(const sf::Time current_time);
private:
    std::shared_ptr<OpenMusic> audio;
    std::atomic<bool> data_is_ready;
    std::vector<float> max_of_every_64_samples;

    void prepare_data();
};