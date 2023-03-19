#include "waveform.hpp"
#include <SFML/Audio/InputSoundFile.hpp>
#include <cmath>
#include <cstddef>
#include "toolbox.hpp"
#include "utf8_sfml.hpp"


namespace waveform {
    ZoomParameters Waveform::zoom_to_params(int zoom) const {
        const AffineTransform<float> zoom_to_index = {
            -10,
            10,
            static_cast<float>(chunk_sizes.size() - 1),
            0
        };
        const auto float_index = zoom_to_index.clampedTransform(zoom);
        const unsigned int chunk_size = chunk_sizes.at(
            std::clamp<std::size_t>(
                std::floor(float_index),
                0,
                chunk_sizes.size() - 1
            )
        );
        const float fractional_chunk_size = std::pow(2, float_index + std::log2(chunk_sizes.at(0)));
        return {
            chunk_size,
            fractional_chunk_size/chunk_size
        };
    }

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

    std::optional<Waveform> compute_waveform(const std::filesystem::path& audio) {
        feis::HoldFileStreamMixin<sf::InputSoundFile> sound_file;
        if (not sound_file.open_from_path(audio)) {
            return {};
        }
        Waveform waveform{
            {},
            {},
            sound_file.getSampleRate(),
            sound_file.getChannelCount()
        };
        unsigned int size = 8;
        waveform.channels_per_chunk_size[size] = load_initial_summary(sound_file, size);
        waveform.chunk_sizes.push_back(size);
        while (waveform.channels_per_chunk_size.size() < 10) {
            size *= 2;
            waveform.channels_per_chunk_size[size] = downsample_to_half(waveform.channels_per_chunk_size.rbegin()->second);
            waveform.chunk_sizes.push_back(size);
        }
        return waveform;
    }
}