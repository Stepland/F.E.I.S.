#include "linear_view_waveform_mode.hpp"


namespace linear_view::mode::waveform {
    Channels load_initial_summary(
        feis::HoldFileStreamMixin<sf::InputSoundFile>& sound_file,
        const unsigned int window_size
    ) {
        const std::size_t chunk_size = window_size * sound_file.getChannelCount();
        const std::size_t point_count = (sound_file.getSampleCount() / sound_file.getChannelCount() / window_size) + 1;
        Channels summary{sound_file.getChannelCount(), DataFrame{point_count}};

        std::vector<sf::Int16> samples;
        samples.resize(chunk_size);
        for (std::size_t point_index = 0; point_index < point_count; point_index++) {
            const auto samples_read = sound_file.read(samples.data(), chunk_size);
            if (samples_read == 0) {
                // we are done reading the file
                break;
            }
            const auto sample_indicies = samples_read / sound_file.getChannelCount();
            for (std::size_t channel_index = 0; channel_index < summary.size(); channel_index++) {
                auto& point = summary[channel_index][point_index];
                point.max = std::numeric_limits<DataPoint::value_type>::min();
                point.min = std::numeric_limits<DataPoint::value_type>::max();
                for (std::size_t sample_index = 0; sample_index < sample_indicies; sample_index++) {
                    auto& sample = samples[sample_index * sound_file.getChannelCount() + channel_index];
                    if (sample > point.max) {
                        point.max = sample;
                    }
                    if (sample < point.min) {
                        point.min = sample;
                    }
                }
            }
            std::ranges::fill(samples, 0);
        }
        return summary;
    };

    Channels downsample_to_half(const Channels& summary) {
        Channels downsampled_summary;
        for (const auto& channel : summary) {
            auto& downsampled_channel = downsampled_summary.emplace_back((channel.size() / 2) + 1);
            auto out = downsampled_channel.begin();
            auto in = channel.begin();
            while (in != channel.end() and out != downsampled_channel.end()) {
                const auto next_in = std::next(in);
                if (next_in != channel.end()) {
                    out->max = std::max(in->max, next_in->max);
                    out->min = std::min(in->min, next_in->min);
                } else {
                    *out = *in;
                    break;
                }
                out = std::next(out);
                std::advance(in, 2);
            }
        }
        return downsampled_summary;
    };

    std::optional<std::reference_wrapper<Channels>>
    WaveformCache::load_waveforms(const std::filesystem::path& audio) {
        auto it =cache.find(audio);
        if (it != cache.end()) {
            return {it->second};
        } else {
            // fire off the loading thread
            return {}
        }
    };
}