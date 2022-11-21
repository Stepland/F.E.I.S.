#include "linear_view_sizes.hpp"

namespace linear_view {
    void Sizes::load_from_v1_0_0_table(const toml::table& linear_view_table) {
        const auto sizes_node = linear_view_table["sizes"];
        timeline_margin = sizes_node["timeline_margin"].value<int>().value_or(timeline_margin);
        cursor_height = sizes_node["cursor_height"].value<int>().value_or(cursor_height);
    }

    void Sizes::dump_as_v1_0_0(toml::table& linear_view_table) {
        toml::table sizes_table{
            {"timeline_margin", timeline_margin},
            {"cursor_height", cursor_height},
        };
        linear_view_table.insert_or_assign("sizes", sizes_table);  
    }
}