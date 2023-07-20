#include "config.hpp"

#include <algorithm>
#include <filesystem>

#include <SFML/Config.hpp>
#include <SFML/System/Time.hpp>
#include <fmt/format.h>
#include <toml++/toml.h>
#include <variant>

#include "color.hpp"
#include "linear_view_colors.hpp"
#include "marker.hpp"
#include "nowide/fstream.hpp"
#include "variant_visitor.hpp"
#include "widgets/lane_order.hpp"
#include "utf8_strings.hpp"

void config::Marker::load_from_v1_0_0_table(const toml::table &tbl) {
    const auto marker_node = tbl["marker"];
    const auto folder_node = marker_node["folder"];
    if (folder_node.is_string()) {
        folder = utf8_encoded_string_to_path(*folder_node.value<std::string>());
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
        marker_table.emplace("folder", path_to_utf8_encoded_string(*folder));
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
    mode = linear_view::mode::load_from_v1_0_0_table(linear_view_table);
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
    linear_view::mode::dump_as_v1_0_0(mode, linear_view_table);
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
}

void config::Editor::dump_as_v1_0_0(toml::table& tbl) {
    tbl.insert_or_assign("editor", toml::table{
        {"collision_zone", collision_zone.asMilliseconds()},
    });
}

void config::Sound::load_from_v1_0_0_table(const toml::table& tbl) {
    if (not tbl["sound"].is_table()) {
        return;
    }
    const auto sound_table = tbl["sound"].ref<toml::table>();
    if (sound_table["music_volume"].is_integer()) {
        const auto val = sound_table["music_volume"].value<int>();
        music_volume = std::clamp(*val, 0, 10);
    }
    if (sound_table["beat_tick"].is_boolean()) {
        beat_tick = *sound_table["beat_tick"].value<bool>();
    }
    if (sound_table["beat_tick_volume"].is_integer()) {
        const auto val = sound_table["beat_tick_volume"].value<int>();
        beat_tick_volume = std::clamp(*val, 0, 10);
    }
    if (sound_table["note_clap"].is_boolean()) {
        note_clap = *sound_table["note_clap"].value<bool>();
    }
    if (sound_table["note_clap_volume"].is_integer()) {
        const auto val = sound_table["note_clap_volume"].value<int>();
        note_clap_volume = std::clamp(*val, 0, 10);
    }
    if (sound_table["clap_on_long_note_ends"].is_boolean()) {
        clap_on_long_note_ends = *sound_table["clap_on_long_note_ends"].value<bool>();
    }
    if (sound_table["distinct_chord_clap"].is_boolean()) {
        distinct_chord_clap = *sound_table["distinct_chord_clap"].value<bool>();
    }
}

void config::Sound::dump_as_v1_0_0(toml::table& tbl) {
    tbl.insert_or_assign("sound", toml::table{
        {"music_volume", music_volume},
        {"beat_tick", beat_tick},
        {"beat_tick_volume", beat_tick_volume},
        {"note_clap", note_clap},
        {"note_clap_volume", note_clap_volume},
        {"clap_on_long_note_ends", clap_on_long_note_ends},
        {"distinct_chord_clap", distinct_chord_clap}
    });
}

void config::Windows::load_from_v1_0_0_table(const toml::table& tbl) {
    if (not tbl["windows"].is_table()) {
        return;
    }
    const auto windows_table = tbl["windows"].ref<toml::table>();
    if (auto val = windows_table["show_playfield"].value<bool>()) {
        show_playfield = *val;
    }
    if (auto val = windows_table["show_playfield_settings"].value<bool>()) {
        show_playfield_settings = *val;
    }
    if (auto val = windows_table["show_file_properties"].value<bool>()) {
        show_file_properties = *val;
    }
    if (auto val = windows_table["show_status"].value<bool>()) {
        show_status = *val;
    }
    if (auto val = windows_table["show_playback_status"].value<bool>()) {
        show_playback_status = *val;
    }
    if (auto val = windows_table["show_timeline"].value<bool>()) {
        show_timeline = *val;
    }
    if (auto val = windows_table["show_chart_list"].value<bool>()) {
        show_chart_list = *val;
    }
    if (auto val = windows_table["show_linear_view"].value<bool>()) {
        show_linear_view = *val;
    }
    if (auto val = windows_table["show_sound_settings"].value<bool>()) {
        show_sound_settings = *val;
    }
    if (auto val = windows_table["show_editor_settings"].value<bool>()) {
        show_editor_settings = *val;
    }
    if (auto val = windows_table["show_history"].value<bool>()) {
        show_history = *val;
    }
    if (auto val = windows_table["show_new_chart_dialog"].value<bool>()) {
        show_new_chart_dialog = *val;
    }
    if (auto val = windows_table["show_chart_properties"].value<bool>()) {
        show_chart_properties = *val;
    }
    if (auto val = windows_table["show_sync_menu"].value<bool>()) {
        show_sync_menu = *val;
    }
    if (auto val = windows_table["show_bpm_change_menu"].value<bool>()) {
        show_bpm_change_menu = *val;
    }
}

void config::Windows::dump_as_v1_0_0(toml::table& tbl) {
    tbl.insert_or_assign("windows", toml::table{
        {"show_playfield", show_playfield},
        {"show_playfield_settings", show_playfield_settings},
        {"show_file_properties", show_file_properties},
        {"show_status", show_status},
        {"show_playback_status", show_playback_status},
        {"show_timeline", show_timeline},
        {"show_chart_list", show_chart_list},
        {"show_linear_view", show_linear_view},
        {"show_sound_settings", show_sound_settings},
        {"show_editor_settings", show_editor_settings},
        {"show_history", show_history},
        {"show_new_chart_dialog", show_new_chart_dialog},
        {"show_chart_properties", show_chart_properties},
        {"show_sync_menu", show_sync_menu},
        {"show_bpm_change_menu", show_bpm_change_menu}
    });
}


void config::Playfield::load_from_v1_0_0_table(const toml::table& tbl) {
    const auto playfield_table = tbl["playfield"];
    if (auto val = playfield_table["show_free_buttons"].value<bool>()) {
        show_free_buttons = *val;
    }
    if (auto val = playfield_table["color_chords"].value<bool>()) {
        color_chords = *val;
    }
    load_color(playfield_table["chord_color"], chord_color);
    if (auto val = playfield_table["chord_color_mix_amount"].value<float>()) {
        chord_color_mix_amount = std::clamp(*val, 0.0f, 1.0f);
    }
    if (auto val = playfield_table["show_note_numbers"].value<bool>()) {
        show_note_numbers = *val;
    }
    if (auto val = playfield_table["note_number_size"].value<float>()) {
        note_number_size = std::clamp(*val, 0.0f, 1.0f);
    }
    load_color(playfield_table["note_number_color"], note_number_color);
    if (auto val = playfield_table["note_number_stroke_width"].value<float>()) {
        note_number_stroke_width = std::clamp(*val, 0.0f, 1.0f);
    }
    load_color(playfield_table["note_number_stroke_color"], note_number_stroke_color);
    const auto& visibility = playfield_table["note_number_visibility_time_span"];
    if (visibility.is_array()) {
        const auto array = visibility.ref<toml::array>();
        if (array.size() == 2) {
            const auto start = array[0].value<int>();
            const auto end = array[1].value<int>();
            if (start and end) {
                note_number_visibility_time_span = {*start, *end};
            }
        }
    }
}

void config::Playfield::dump_as_v1_0_0(toml::table& tbl) {
    tbl.insert_or_assign("playfield", toml::table{
        {"show_free_buttons", show_free_buttons},
        {"color_chords", color_chords},
        {"chord_color", dump_color(chord_color)},
        {"chord_color_mix_amount", chord_color_mix_amount},
        {"show_note_numbers", show_note_numbers},
        {"note_number_size", note_number_size},
        {"note_number_color", dump_color(note_number_color)},
        {"note_number_stroke_width", note_number_stroke_width},
        {"note_number_stroke_color", dump_color(note_number_stroke_color)},
        {"note_number_visibility_time_span", toml::array{note_number_visibility_time_span.start, note_number_visibility_time_span.end}}
    });
}

config::Config::Config(const std::filesystem::path& settings) :
    config_path(settings / "config.toml")
{
    if (not std::filesystem::exists(config_path)) {
        return;
    }
    
    toml::table tbl;
    nowide::ifstream config_stream{path_to_utf8_encoded_string(config_path)};
    try {
        tbl = toml::parse(config_stream);
    } catch (const toml::parse_error& err) {
        fmt::print(
            "Error while parsing {} :\n{}",
            path_to_utf8_encoded_string(settings),
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
    sound.dump_as_v1_0_0(tbl);
    windows.dump_as_v1_0_0(tbl);
    playfield.dump_as_v1_0_0(tbl);
    return tbl;
}

config::Config::~Config() {
    std::filesystem::create_directories(config_path.parent_path());
    const auto tbl = dump_as_v1_0_0();
    nowide::ofstream config_file_stream{path_to_utf8_encoded_string(config_path)};
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
    sound.load_from_v1_0_0_table(tbl);
    windows.load_from_v1_0_0_table(tbl);
    playfield.load_from_v1_0_0_table(tbl);
}