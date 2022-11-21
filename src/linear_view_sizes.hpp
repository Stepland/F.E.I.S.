#pragma once

#include <toml++/toml.h>

namespace linear_view {
    struct Sizes {
        int timeline_margin = 130;
        int cursor_height = 100;

        void load_from_v1_0_0_table(const toml::table& linear_view_table);
        void dump_as_v1_0_0(toml::table& linear_view_table);
    };

    const Sizes default_sizes = {};
}