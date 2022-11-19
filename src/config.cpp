#include "config.hpp"

#include <SFML/Config.hpp>
#include <SFML/System/Time.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <toml++/toml.h>
#include <variant>

#include "colors.hpp"
#include "marker.hpp"
#include "variant_visitor.hpp"
#include "widgets/lane_order.hpp"

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

linear_view::Colors config::load_linear_view_colors_from_v1_0_0_table(const toml::table& linear_view) {
    auto colors = linear_view::default_colors;
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

void config::dump_linear_view_colors_as_v1_0_0(const linear_view::Colors& colors, toml::table& linear_view) {
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

linear_view::Sizes config::load_linear_view_sizes_from_v1_0_0_table(const toml::table& linear_view) {
    auto sizes = linear_view::default_sizes;
    const auto sizes_node = linear_view["sizes"];
    sizes.timeline_margin = sizes_node["timeline_margin"].value<int>().value_or(sizes.timeline_margin);
    sizes.cursor_height = sizes_node["cursor_height"].value<int>().value_or(sizes.cursor_height);
    return sizes;
}

void config::dump_linear_view_sizes_as_v1_0_0(const linear_view::Sizes& sizes, toml::table& linear_view) {
    toml::table sizes_table{
        {"timeline_margin", sizes.timeline_margin},
        {"cursor_height", sizes.cursor_height},
    };
    linear_view.insert_or_assign("sizes", sizes_table);  
}

linear_view::LaneOrder config::load_linear_view_lane_order_from_v1_0_0_table(const toml::table& linear_view) {
    auto lane_order = linear_view::default_lane_order;
    const auto lane_order_node = linear_view["lane_order"];
    const auto type = lane_order_node["type"].value<std::string>();
    if (not type) {
        return lane_order;
    }
    
    if (*type == "default") {
        return linear_view::lane_order::Default{};
    } else if (*type == "vertical") {
        return linear_view::lane_order::Vertical{};
    } else if (*type == "custom") {
        const auto order_as_string = lane_order_node["order"].value<std::string>();
        if (order_as_string) {
            return linear_view::lane_order::Custom{*order_as_string};
        }
    }
    
    return lane_order;


}

void config::dump_linear_view_lane_order_as_v1_0_0(const linear_view::LaneOrder& lane_order, toml::table& linear_view) {
    const auto _dump = VariantVisitor {
        [&](const linear_view::lane_order::Default&) {
            linear_view.insert_or_assign(
                "lane_order",
                toml::table{{"type", "default"}}
            );
        },
        [&](const linear_view::lane_order::Vertical&) {
            linear_view.insert_or_assign(
                "lane_order",
                toml::table{{"type", "vertical"}}
            );
        },
        [&](const linear_view::lane_order::Custom& custom) {
            linear_view.insert_or_assign(
                "lane_order",
                toml::table{
                    {"type", "custom"},
                    {"order", custom.as_string}
                }
            );
        }
    };
    std::visit(_dump, lane_order);
}

void config::LinearView::load_from_v1_0_0_table(const toml::table& tbl) {
    if (not tbl["linear_view"].is_table()) {
        return;
    }
    const auto linear_view_table = tbl["linear_view"].ref<toml::table>();
    colors = load_linear_view_colors_from_v1_0_0_table(linear_view_table);
    sizes = load_linear_view_sizes_from_v1_0_0_table(linear_view_table);
    lane_order = load_linear_view_lane_order_from_v1_0_0_table(linear_view_table);
    if (linear_view_table["zoom"].is_integer()) {
        zoom = *linear_view_table["zoom"].value<int>();
    }
}

void config::LinearView::dump_as_v1_0_0(toml::table& tbl) {
    toml::table linear_view;
    dump_linear_view_colors_as_v1_0_0(colors, linear_view);
    dump_linear_view_sizes_as_v1_0_0(sizes, linear_view);
    dump_linear_view_lane_order_as_v1_0_0(lane_order, linear_view);
    linear_view.insert_or_assign("zoom", zoom);
    tbl.insert_or_assign("linear_view", linear_view);
}


void config::Editor::load_from_v1_0_0_table(const toml::table& tbl) {
    if (not tbl["editor"].is_table()) {
        return;
    }
    const auto editor_table = tbl["editor"].ref<toml::table>();
    if (editor_table["collision_zone"].is_integer()) {
        const auto ms = editor_table["collision_zone"].value<int>();
        collision_zone = sf::milliseconds(std::clamp(*ms, 100, 2000));
    }

}

void config::Editor::dump_as_v1_0_0(toml::table& tbl) {
    tbl.insert_or_assign("editor", toml::table{{
        "collision_zone", collision_zone.asMilliseconds()
    }});
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
    editor.dump_as_v1_0_0(tbl);
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
    editor.load_from_v1_0_0_table(tbl);
}