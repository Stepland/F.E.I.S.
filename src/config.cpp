#include "config.hpp"

#include <filesystem>

#include <SFML/Config.hpp>
#include <SFML/System/Time.hpp>
#include <fmt/format.h>
#include <toml++/toml.h>
#include <variant>

#include "linear_view_colors.hpp"
#include "marker.hpp"
#include "variant_visitor.hpp"
#include "widgets/lane_order.hpp"

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

void config::LinearView::load_from_v1_0_0_table(const toml::table& tbl) {
    if (not tbl["linear_view"].is_table()) {
        return;
    }
    const auto linear_view_table = tbl["linear_view"].ref<toml::table>();
    colors.load_from_v1_0_0_table(linear_view_table);
    sizes.load_from_v1_0_0_table(linear_view_table);
    lane_order = linear_view::lane_order::load_from_v1_0_0_table(linear_view_table);
    if (linear_view_table["zoom"].is_integer()) {
        zoom = *linear_view_table["zoom"].value<int>();
    }
    if (linear_view_table["use_quantization_colors"].is_boolean()) {
        use_quantization_colors = *linear_view_table["use_quantization_colors"].value<bool>();
    }
    quantization_colors.load_from_v1_0_0_table(linear_view_table);
}

void config::LinearView::dump_as_v1_0_0(toml::table& tbl) {
    toml::table linear_view_table;
    colors.dump_as_v1_0_0(linear_view_table);
    sizes.dump_as_v1_0_0(linear_view_table);
    linear_view::lane_order::dump_as_v1_0_0(lane_order, linear_view_table);
    linear_view_table.insert_or_assign("zoom", zoom);
    linear_view_table.insert_or_assign("use_quantization_colors", use_quantization_colors);
    quantization_colors.dump_as_v1_0_0(linear_view_table);
    tbl.insert_or_assign("linear_view", linear_view_table);
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
    if (editor_table["show_free_buttons"].is_boolean()) {
        show_free_buttons = *editor_table["show_free_buttons"].value<bool>();
    }
}

void config::Editor::dump_as_v1_0_0(toml::table& tbl) {
    tbl.insert_or_assign("editor", toml::table{
        {"collision_zone", collision_zone.asMilliseconds()},
        {"show_free_buttons", show_free_buttons}
    });
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
            config_path.string(),
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
    const auto flags = (
        toml::toml_formatter::default_flags
        & ~toml::format_flags::allow_literal_strings
        & ~toml::format_flags::indent_sub_tables
    );
    config_file_stream << toml::toml_formatter{tbl, flags} << std::endl;
}

void config::Config::load_from_v1_0_0_table(const toml::table& tbl) {
    marker.load_from_v1_0_0_table(tbl);
    linear_view.load_from_v1_0_0_table(tbl);
    editor.load_from_v1_0_0_table(tbl);
}