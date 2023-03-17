#include "linear_view_mode.hpp"

#include "variant_visitor.hpp"

namespace linear_view {
    Mode load_from_v1_0_0_table(const toml::table& linear_view) {
        const auto mode_string = linear_view["mode"].value<std::string>();
        if (not mode_string) {
            return linear_view::default_mode;
        }
        
        if (*mode_string == "beats") {
            return mode::Beats{};
        } else if (*mode_string == "waveform") {
            return mode::Waveform{};
        }
        
        return linear_view::default_mode;
    }

    void dump_as_v1_0_0(const Mode& mode, toml::table& linear_view) {
        const auto _dump = VariantVisitor {
            [&](const mode::Beats&) {
                linear_view.insert_or_assign("mode", "beats");
            },
            [&](const mode::Waveform&) {
                linear_view.insert_or_assign("mode", "waveform");
            },
        };
        std::visit(_dump, mode);
    }
}