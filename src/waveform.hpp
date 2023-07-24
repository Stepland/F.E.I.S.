#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <variant>
#include <vector>

#include <SFML/Audio/InputSoundFile.hpp>

#include "utf8_sfml_redefinitions.hpp"
#include "variant_visitor.hpp"

namespace waveform {
    struct DataPoint {
        using value_type = std::int16_t;
        value_type min = 0;
        value_type max = 0;
    };

    using DataFrame = std::vector<DataPoint>;
    using Channels = std::vector<DataFrame>;

    struct ChunkSizes {
        unsigned int reference;
        float fractional;
        float frac_to_ref_ratio;
    };

    struct Waveform {
        std::map<unsigned int, Channels> channels_per_chunk_size;
        std::vector<unsigned int> chunk_sizes; // for fast nth chunk size access
        std::size_t sample_rate;
        std::size_t channel_count;

        ChunkSizes zoom_to_params(int zoom) const;
    };

    Channels load_initial_summary(
        feis::InputSoundFile& sound_file,
        const unsigned int window_size
    );
    Channels downsample_to_half(const Channels& summary);
    std::optional<Waveform> compute_waveform(const std::filesystem::path& audio);

    namespace status {
        struct NoAudioFile {};
        struct Loading {};
        struct ErrorDuringLoading {};
        struct Loaded {
            Waveform& waveform;
        };
    }

    using Status = std::variant<
        status::NoAudioFile,
        status::Loading,
        status::ErrorDuringLoading,
        status::Loaded
    >;

    const auto status_message = VariantVisitor {
        [](status::NoAudioFile) -> std::optional<std::string> {return "No Audio File";},
        [](status::Loading) -> std::optional<std::string> {return "Loading ...";},
        [](status::ErrorDuringLoading) -> std::optional<std::string> {return "Error while loading waveform";},
        [](status::Loaded) -> std::optional<std::string> {return {};},
    };
}