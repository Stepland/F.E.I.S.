#include "config.hpp"

#include <SFML/Config.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <toml++/toml.h>

#include "colors.hpp"
#include "marker.hpp"

toml::array config::dump_color(const sf::Color& color) {
    return toml::array{color.r, color.g, color.b, color.a};
}

std::optional<sf::Uint8> parse_uint8(const toml::node& node) {
    const auto raw_value_opt = node.value<int>();
    if (not raw_value_opt) {
        return {};
    }
    const auto raw_value = *raw_value_opt;
    if (raw_value < 0 or raw_value > 255) {
        return {};
    }
    return static_cast<sf::Uint8>(raw_value);
}

std::optional<sf::Color> config::parse_color(const node_view& node) {
    if (not node.is_array()) {
        return {};
    }
    const auto array = node.ref<toml::array>();
    if (array.size() != 4) {
        return {};
    }
    const auto r = parse_uint8(array[0]);
    if (not r) {
        return {};
    }
    const auto g = parse_uint8(array[1]);
    if (not g) {
        return {};
    }
    const auto b = parse_uint8(array[2]);
    if (not b) {
        return {};
    }
    const auto a = parse_uint8(array[3]);
    if (not a) {
        return {};
    }
    return sf::Color(*r, *g, *b, *a);
}

void config::load_color(const node_view& node, sf::Color& color) {
    const auto parsed = config::parse_color(node);
    if (parsed) {
        color = *parsed;
    }
}

void config::Marker::load_from_v1_0_0_table(const toml::table &tbl) {
    const auto marker_node = tbl["marker"];
    const auto folder_node = marker_node["folder"];
    if (folder_node.is_string()) {
        folder = std::filesystem::path{*folder_node.value<std::string>()};
    }
    const auto ending_state_node = marker_node["ending_state"];
    if (ending_state_node.is_string()) {
        const auto name = *ending_state_node.value<std::string>();
        const auto it = name_to_judgement.find(name);
        if (it != name_to_judgement.end()) {
            ending_state = it->second;
        }
    }
}

void config::Marker::dump_as_v1_0_0(toml::table &tbl) {
    toml::table marker_table;
    if (folder) {
        marker_table.emplace("folder", folder->string());
    }
    if (ending_state) {
        marker_table.emplace("ending_state", judgement_to_name.at(*ending_state));
    }
    tbl.insert_or_assign("marker", marker_table);
}

LinearViewColors config::load_linear_view_colors_from_v1_0_0_table(const toml::table& linear_view) {
    auto colors = default_linear_view_colors;
    const auto colors_node = linear_view["colors"];
    load_color(colors_node["cursor"], colors.cursor);
    load_color(colors_node["tab_selection"]["fill"], colors.tab_selection.fill);
    load_color(colors_node["tab_selection"]["border"], colors.tab_selection.border);
    load_color(colors_node["normal_tap_note"], colors.normal_tap_note);
    load_color(colors_node["conflicting_tap_note"], colors.conflicting_tap_note);
    load_color(colors_node["normal_collision_zone"], colors.normal_collision_zone);
    load_color(colors_node["conflicting_collision_zone"], colors.conflicting_collision_zone);
    load_color(colors_node["normal_long_note"], colors.normal_long_note);
    load_color(colors_node["conflicting_long_note"], colors.conflicting_long_note);
    load_color(colors_node["selected_note_fill"], colors.selected_note_fill);
    load_color(colors_node["selected_note_outline"], colors.selected_note_outline);
    load_color(colors_node["measure_line"], colors.measure_line);
    load_color(colors_node["measure_number"], colors.measure_number);
    load_color(colors_node["beat_line"], colors.beat_line);
    load_color(colors_node["bpm_button"]["text"], colors.bpm_button.text);
    load_color(colors_node["bpm_button"]["button"], colors.bpm_button.button);
    load_color(colors_node["bpm_button"]["hover"], colors.bpm_button.hover);
    load_color(colors_node["bpm_button"]["active"], colors.bpm_button.active);
    load_color(colors_node["bpm_button"]["border"], colors.bpm_button.border);
    load_color(colors_node["selection_rect"]["fill"], colors.selection_rect.fill);
    load_color(colors_node["selection_rect"]["border"], colors.selection_rect.border);
    return colors;
}

void config::dump_linear_view_colors_as_v1_0_0(const LinearViewColors& colors, toml::table& linear_view) {
    toml::table colors_table{
        {"cursor", dump_color(colors.cursor)},
        {"tab_selection", toml::table{
            {"fill", dump_color(colors.tab_selection.fill)},
            {"border", dump_color(colors.tab_selection.border)},
        }},
        {"normal_tap_note", dump_color(colors.normal_tap_note)},
        {"conflicting_tap_note", dump_color(colors.conflicting_tap_note)},
        {"normal_collision_zone", dump_color(colors.normal_collision_zone)},
        {"conflicting_collision_zone", dump_color(colors.conflicting_collision_zone)},
        {"normal_long_note", dump_color(colors.normal_long_note)},
        {"conflicting_long_note", dump_color(colors.conflicting_long_note)},
        {"selected_note_fill", dump_color(colors.selected_note_fill)},
        {"selected_note_outline", dump_color(colors.selected_note_outline)},
        {"measure_line", dump_color(colors.measure_line)},
        {"measure_number", dump_color(colors.measure_number)},
        {"beat_line", dump_color(colors.beat_line)},
        {"bpm_button", toml::table{
            {"text", dump_color(colors.bpm_button.text)},
            {"button", dump_color(colors.bpm_button.button)},
            {"hover", dump_color(colors.bpm_button.hover)},
            {"active", dump_color(colors.bpm_button.active)},
            {"border", dump_color(colors.bpm_button.border)},
        }},
        {"selection_rect", toml::table{
            {"fill", dump_color(colors.selection_rect.fill)},
            {"border", dump_color(colors.selection_rect.border)},
        }}
    };
    linear_view.insert_or_assign("colors", colors_table);   
}

void config::LinearView::load_from_v1_0_0_table(const toml::table& tbl) {
    if (tbl["linear_view"].is_table()) {
        colors = load_linear_view_colors_from_v1_0_0_table(tbl["linear_view"].ref<toml::table>());
    }
}

void config::LinearView::dump_as_v1_0_0(toml::table& tbl) {
    toml::table linear_view;
    dump_linear_view_colors_as_v1_0_0(colors, linear_view);
    tbl.insert_or_assign("linear_view", linear_view);
}



config::Config::Config(const std::filesystem::path& settings) :
    config_path(settings / "config.toml")
{
    if (not std::filesystem::exists(config_path)) {
        return;
    }
    
    toml::table tbl;
    try {
        tbl = toml::parse_file(config_path.c_str());
    } catch (const toml::parse_error& err) {
        fmt::print(
            "Error while parsing {} :\n{}",
            config_path.c_str(),
            err.what()
        );
        return;
    }

    const auto file_version = tbl["version"].value<std::string>();
    if (not file_version) {
        return;
    }

    if (*file_version == "1.0.0") {
        load_from_v1_0_0_table(tbl);
    }
}

toml::table config::Config::dump_as_v1_0_0() {
    auto tbl = toml::table{
        {"version", version}
    };
    marker.dump_as_v1_0_0(tbl);
    linear_view.dump_as_v1_0_0(tbl);
    return tbl;
}

config::Config::~Config() {
    std::filesystem::create_directories(config_path.parent_path());
    const auto tbl = dump_as_v1_0_0();
    std::ofstream config_file_stream{config_path};
    config_file_stream << tbl << std::endl;
}

void config::Config::load_from_v1_0_0_table(const toml::table& tbl) {
    marker.load_from_v1_0_0_table(tbl);
    linear_view.load_from_v1_0_0_table(tbl);
}