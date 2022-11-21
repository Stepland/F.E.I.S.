#include "linear_view_colors.hpp"

#include "color.hpp"

namespace linear_view {
    void Colors::load_from_v1_0_0_table(const toml::table& linear_view_table) {
        const auto colors_node = linear_view_table["colors"];
        load_color(colors_node["cursor"], cursor);
        load_color(colors_node["tab_selection"]["fill"], tab_selection.fill);
        load_color(colors_node["tab_selection"]["border"], tab_selection.border);
        load_color(colors_node["normal_tap_note"], normal_tap_note);
        load_color(colors_node["conflicting_tap_note"], conflicting_tap_note);
        load_color(colors_node["normal_collision_zone"], normal_collision_zone);
        load_color(colors_node["conflicting_collision_zone"], conflicting_collision_zone);
        load_color(colors_node["normal_long_note"], normal_long_note);
        load_color(colors_node["conflicting_long_note"], conflicting_long_note);
        load_color(colors_node["selected_note_fill"], selected_note_fill);
        load_color(colors_node["selected_note_outline"], selected_note_outline);
        load_color(colors_node["measure_line"], measure_line);
        load_color(colors_node["measure_number"], measure_number);
        load_color(colors_node["beat_line"], beat_line);
        load_color(colors_node["bpm_button"]["text"], bpm_button.text);
        load_color(colors_node["bpm_button"]["button"], bpm_button.button);
        load_color(colors_node["bpm_button"]["hover"], bpm_button.hover);
        load_color(colors_node["bpm_button"]["active"], bpm_button.active);
        load_color(colors_node["bpm_button"]["border"], bpm_button.border);
        load_color(colors_node["selection_rect"]["fill"], selection_rect.fill);
        load_color(colors_node["selection_rect"]["border"], selection_rect.border);
    }

    void Colors::dump_as_v1_0_0(toml::table& linear_view_table) {
        toml::table colors_table{
            {"cursor", dump_color(cursor)},
            {"tab_selection", toml::table{
                {"fill", dump_color(tab_selection.fill)},
                {"border", dump_color(tab_selection.border)},
            }},
            {"normal_tap_note", dump_color(normal_tap_note)},
            {"conflicting_tap_note", dump_color(conflicting_tap_note)},
            {"normal_collision_zone", dump_color(normal_collision_zone)},
            {"conflicting_collision_zone", dump_color(conflicting_collision_zone)},
            {"normal_long_note", dump_color(normal_long_note)},
            {"conflicting_long_note", dump_color(conflicting_long_note)},
            {"selected_note_fill", dump_color(selected_note_fill)},
            {"selected_note_outline", dump_color(selected_note_outline)},
            {"measure_line", dump_color(measure_line)},
            {"measure_number", dump_color(measure_number)},
            {"beat_line", dump_color(beat_line)},
            {"bpm_button", toml::table{
                {"text", dump_color(bpm_button.text)},
                {"button", dump_color(bpm_button.button)},
                {"hover", dump_color(bpm_button.hover)},
                {"active", dump_color(bpm_button.active)},
                {"border", dump_color(bpm_button.border)},
            }},
            {"selection_rect", toml::table{
                {"fill", dump_color(selection_rect.fill)},
                {"border", dump_color(selection_rect.border)},
            }}
        };
        linear_view_table.insert_or_assign("colors", colors_table); 
    }
}
