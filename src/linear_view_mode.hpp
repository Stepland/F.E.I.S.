#pragma once

#include <variant>

#include <toml++/toml.h>

namespace linear_view {
    namespace mode {
        struct Beats {};
        struct Waveform {};
    }

    using Mode = std::variant<mode::Beats, mode::Waveform>;

    namespace mode {
        linear_view::Mode load_from_v1_0_0_table(const toml::table& linear_view);
        void dump_as_v1_0_0(const linear_view::Mode& mode, toml::table& linear_view);
    }

    const Mode default_mode = mode::Beats{};
}