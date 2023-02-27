#include "linear_view.hpp"

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <bits/ranges_algo.h>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <ranges>
#include <sstream>
#include <variant>

#include <fmt/core.h>
#include <hsluv/hsluv.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>
#include <SFML/System/Time.hpp>

#include "../better_timing.hpp"
#include "../better_note.hpp"
#include "../chart_state.hpp"
#include "../imgui_extras.hpp"
#include "../long_note_dummy.hpp"
#include "../special_numeric_types.hpp"
#include "../toolbox.hpp"
#include "../variant_visitor.hpp"
#include "widgets/lane_order.hpp"


const std::string font_file = "fonts/NotoSans-Medium.ttf";

void SelectionRectangle::reset() {
    start = {-1, -1};
    end = {-1, -1};
}

namespace linear_view::mode {
    Waveform::Waveform(const std::filesystem::path& audio) {
        sound_file.open_from_path(audio);
        worker = std::jthread{&Waveform::prepare_data, this};
    }

    void Waveform::draw_waveform(
        ImDrawList* draw_list,
        const sf::Time current_time,
        int zoom
    ) {
        if (ImGui::Begin("Waveform view")) {
            if (not data_is_ready) {
                feis::CenteredText("Loading ...");
                return ImGui::End();
            }
            if (channels_per_chunk_size.empty()) {
                feis::CenteredText("No data ???");
                return ImGui::End();
            }

            zoom = std::clamp(zoom, 0, static_cast<int>(channels_per_chunk_size.size()) - 1);
            const auto& channels_it = channels_per_chunk_size.at(channels_per_chunk_size.size() - zoom - 1);
            const auto& [chunk_size, channels] = channels_it;
            const auto window = ImGui::GetCurrentWindow();
            const auto work_rect = window->WorkRect;
            const float waveform_w_margin = 10.f;
            const float waveform_bounding_width = work_rect.GetWidth() / channels.size();
            const float waveform_width = waveform_bounding_width - waveform_w_margin;
            const AffineTransform<float> value_to_pixel_offset_from_waveform_center{
                std::numeric_limits<DataPoint::value_type>::min(),
                std::numeric_limits<DataPoint::value_type>::max(),
                -waveform_width / 2,
                waveform_width / 2
            };
            const float cursor_y = 50.f;
            for (std::size_t channel_index = 0; channel_index < channels.size(); channel_index++) {
                const auto& data_points = channels[channel_index];
                const std::int64_t sample_at_cursor = time_to_samples(current_time, sound_file.getSampleRate(), sound_file.getChannelCount());
                const auto chunk_at_cursor = sample_at_cursor / chunk_size / sound_file.getChannelCount();
                const auto first_chunk = chunk_at_cursor - static_cast<std::int64_t>(cursor_y);
                const auto end_chunk = first_chunk + static_cast<std::int64_t>(work_rect.GetHeight());
                const auto waveform_x_center = channel_index * waveform_bounding_width + waveform_bounding_width / 2;
                for (std::int64_t data_point_index = first_chunk; data_point_index < end_chunk; data_point_index++) {
                    if (data_point_index < 0 or static_cast<std::size_t>(data_point_index) >= data_points.size()) {
                        continue;
                    }
                    const auto& data_point = data_points[data_point_index];
                    const auto y = work_rect.Min.y + data_point_index - first_chunk;
                    const auto x_offset_min = value_to_pixel_offset_from_waveform_center.transform(data_point.min);
                    const auto x_offset_max = value_to_pixel_offset_from_waveform_center.transform(data_point.max);
                    const auto x_min = work_rect.Min.x + waveform_x_center + x_offset_min;
                    const auto x_max = work_rect.Min.x + waveform_x_center + x_offset_max;
                    draw_list->AddLine({x_min, y}, {x_max, y}, ImColor(sf::Color::White));
                }
            }
        }
        ImGui::End();
    }

    void Waveform::prepare_data() {
        unsigned int size = 8;
        channels_per_chunk_size.emplace_back(
            size,
            waveform::load_initial_summary(sound_file, size)
        );
        while (channels_per_chunk_size.size() < 10) {
            channels_per_chunk_size.emplace_back(
                size * 2,
                waveform::downsample_to_half(channels_per_chunk_size.rbegin()->second)
            );
            size *= 2;
        }
        data_is_ready = true;
    }
}

namespace linear_view::mode::waveform {
    Channels load_initial_summary(
        feis::HoldFileStreamMixin<sf::InputSoundFile>& sound_file,
        const unsigned int window_size
    ) {
        const std::size_t chunk_size = window_size * sound_file.getChannelCount();
        const std::size_t point_count = (sound_file.getSampleCount() / sound_file.getChannelCount() / window_size) + 1;
        Channels summary{sound_file.getChannelCount(), DataFrame{point_count}};

        std::vector<sf::Int16> samples;
        samples.resize(chunk_size);
        for (std::size_t point_index = 0; point_index < point_count; point_index++) {
            const auto samples_read = sound_file.read(samples.data(), chunk_size);
            if (samples_read == 0) {
                // we are done reading the file
                break;
            }
            const auto sample_indicies = samples_read / sound_file.getChannelCount();
            for (std::size_t channel_index = 0; channel_index < summary.size(); channel_index++) {
                auto& point = summary[channel_index][point_index];
                point.max = std::numeric_limits<DataPoint::value_type>::min();
                point.min = std::numeric_limits<DataPoint::value_type>::max();
                for (std::size_t sample_index = 0; sample_index < sample_indicies; sample_index++) {
                    auto& sample = samples[sample_index * sound_file.getChannelCount() + channel_index];
                    if (sample > point.max) {
                        point.max = sample;
                    }
                    if (sample < point.min) {
                        point.min = sample;
                    }
                }
            }
            std::ranges::fill(samples, 0);
        }
        return summary;
    }

    Channels downsample_to_half(const Channels& summary) {
        Channels downsampled_summary;
        for (const auto& channel : summary) {
            auto& downsampled_channel = downsampled_summary.emplace_back((channel.size() / 2) + 1);
            auto out = downsampled_channel.begin();
            auto in = channel.begin();
            while (in != channel.end() and out != downsampled_channel.end()) {
                const auto next_in = std::next(in);
                if (next_in != channel.end()) {
                    out->max = std::max(in->max, next_in->max);
                    out->min = std::min(in->min, next_in->min);
                } else {
                    *out = *in;
                    break;
                }
                out = std::next(out);
                std::advance(in, 2);
            }
        }
        return downsampled_summary;
    };
}

LinearView::LinearView(std::filesystem::path assets, config::Config& config_) :
    colors(config_.linear_view.colors),
    sizes(config_.linear_view.sizes),
    collision_zone(config_.editor.collision_zone),
    beats_to_pixels_proportional(0, 1, 0, 100),
    zoom(config_.linear_view.zoom),
    use_quantization_colors(config_.linear_view.use_quantization_colors),
    quantization_colors(config_.linear_view.quantization_colors),
    lane_order(config_.linear_view.lane_order)
{
    set_zoom(config_.linear_view.zoom);
}

void LinearView::draw(
    ImDrawList* draw_list,
    ChartState& chart_state,
    const better::Timing& timing,
    const Fraction& current_beat,
    const Fraction& last_editable_beat,
    const Fraction& snap,
    const sf::Vector2f& size,
    const sf::Vector2f& origin
) {
    int x = std::max(300, static_cast<int>(size.x));
    int y = std::max(300, static_cast<int>(size.y));

    const float timeline_width = static_cast<float>(x) - static_cast<float>(sizes.timeline_margin);
    const float timeline_left = static_cast<float>(sizes.timeline_margin) / 2;
    const float timeline_right = timeline_left + timeline_width;

    // Just in case, clamp the beat cursor inside the window, with some margin
    const float cursor_y = std::clamp(static_cast<float>(sizes.cursor_height), 25.f, static_cast<float>(y) - 25.f);

    // Here we compute the range of visible beats from the size of the window
    // in pixels, we know by definition that the current beat is exactly at
    // cursor_y pixels and we use this fact to compute the rest
    const auto beats_before_cursor = beats_to_pixels_proportional.backwards_transform(cursor_y);
    const auto beats_after_cursor = beats_to_pixels_proportional.backwards_transform(static_cast<float>(y) - cursor_y);
    const Fraction first_beat_in_frame = current_beat - beats_before_cursor;
    const Fraction last_beat_in_frame = current_beat + beats_after_cursor;
    AffineTransform<Fraction> beats_to_pixels_absolute{first_beat_in_frame, last_beat_in_frame, 0, y};

    const Fraction first_visible_beat = std::max(Fraction{0}, first_beat_in_frame);
    auto next_beat = [](const auto& first_beat) -> Fraction {
        if (first_beat % 1 == 0) {
            return first_beat;
        } else {
            return floor_fraction(first_beat) + 1;
        }
    }(first_visible_beat);

    // Draw the beat lines and numbers
    for (
        Fraction next_beat_line_y = beats_to_pixels_absolute.transform(next_beat);
        next_beat_line_y < y and next_beat < last_editable_beat;
        next_beat_line_y = beats_to_pixels_absolute.transform(next_beat += 1)
    ) {
        const sf::Vector2f beat_line_start = {timeline_left, static_cast<float>(static_cast<double>(next_beat_line_y))};
        const sf::Vector2f beat_line_end = beat_line_start + sf::Vector2f{timeline_width, 0};
        if (next_beat % 4 == 0) {
            draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(colors.measure_line));
            const Fraction measure = next_beat / 4;
            const auto measure_string = fmt::format("{}", static_cast<std::int64_t>(measure));
            const sf::Vector2f text_size = ImGui::CalcTextSize(measure_string.c_str(), measure_string.c_str()+measure_string.size());
            const sf::Vector2f measure_text_pos = {timeline_left - 10, static_cast<float>(static_cast<double>(next_beat_line_y))};
            draw_list->AddText(
                origin + measure_text_pos - sf::Vector2f{text_size.x, text_size.y * 0.5f},
                ImColor(colors.measure_number),
                measure_string.c_str(),
                measure_string.c_str() + measure_string.size()
            );
        } else {
            draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(colors.beat_line));
        }
    }

    const float bpm_events_left = timeline_right + 10;

    // Draw the bpm changes
    timing.for_each_event_between(
        first_beat_in_frame,
        last_beat_in_frame,
        [&](const auto& event){
            const auto bpm_change_y = beats_to_pixels_absolute.transform(event.get_beats());
            if (bpm_change_y >= 0 and bpm_change_y <= y) {
                const sf::Vector2f bpm_text_raw_pos = {bpm_events_left, static_cast<float>(static_cast<double>(bpm_change_y))};
                const auto bpm_at_beat = better::BPMAtBeat{event.get_bpm(), event.get_beats()};
                const auto selected = chart_state.selected_stuff.bpm_events.contains(bpm_at_beat);
                if (BPMButton(event, selected, bpm_text_raw_pos, colors.bpm_button)) {
                    if (selected) {
                        chart_state.selected_stuff.bpm_events.erase(bpm_at_beat);
                    } else {
                        chart_state.selected_stuff.bpm_events.insert(bpm_at_beat);
                    }
                }
            }
        }
    );

    float note_width = timeline_width / 16.f;
    float collizion_zone_width = note_width - 2.f;
    float long_note_rect_width = note_width * 0.75f;

    // Pre-size & center the shapes that can be
    const sf::Vector2f note_size = {note_width, 6.f};

    const sf::Vector2f selected_note_size = {note_width + 2.f, 8.f};

    // Draw the notes
    auto draw_note = VariantVisitor {
        [&](const better::TapNote& tap_note){
            const auto opt_lane = button_to_lane(tap_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            const float note_x = timeline_left + note_width * (lane + 0.5f);
            const float note_y = static_cast<double>(beats_to_pixels_absolute.transform(tap_note.get_time()));
            const auto note_seconds = timing.time_at(tap_note.get_time());
            const auto first_colliding_beat = timing.beats_at(note_seconds - collision_zone * 0.5f);
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto last_colliding_beat = timing.beats_at(note_seconds + collision_zone * 0.5f);
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            const sf::Vector2f collision_zone_pos = {
                note_x,
                static_cast<float>(static_cast<double>(collision_zone_y))
            };
            const sf::Vector2f collizion_zone_size = {
                collizion_zone_width,
                static_cast<float>(static_cast<double>(collision_zone_height))
            };
            const auto collision_zone_color = [&](){
                if (chart_state.chart.notes->is_colliding(tap_note, timing, collision_zone)) {
                    return colors.conflicting_collision_zone;
                } else {    
                    return colors.normal_collision_zone;
                }
            }();
            const auto tap_note_color = [&](){
                if (use_quantization_colors) {
                    return quantization_colors.color_at_beat(tap_note.get_time());
                } else if (chart_state.chart.notes->is_colliding(tap_note, timing, collision_zone)) {
                    return colors.conflicting_tap_note;
                } else {
                    return colors.normal_tap_note;
                }
            }();
            draw_rectangle(
                draw_list,
                origin + collision_zone_pos,
                collizion_zone_size,
                {0.5f, 0.f},
                collision_zone_color
            );
            const sf::Vector2f note_pos = {note_x, note_y};
            draw_rectangle(
                draw_list,
                origin + note_pos,
                note_size,
                {0.5f, 0.5f},
                tap_note_color
            );
            if (chart_state.selected_stuff.notes.contains(tap_note)) {
                draw_rectangle(
                    draw_list,
                    origin + note_pos,
                    selected_note_size,
                    {0.5f, 0.5f},
                    colors.selected_note_fill,
                    colors.selected_note_outline
                );
            }
        },
        [&](const better::LongNote& long_note){
            const auto opt_lane = button_to_lane(long_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            float note_x = timeline_left + note_width * (lane + 0.5f);
            float note_y = static_cast<double>(beats_to_pixels_absolute.transform(long_note.get_time()));
            const auto note_start_seconds = timing.time_at(long_note.get_time());
            const auto first_colliding_beat = timing.beats_at(note_start_seconds - collision_zone * 0.5f);
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto note_end_seconds = timing.time_at(long_note.get_end());
            const auto last_colliding_beat = timing.beats_at(note_end_seconds + collision_zone * 0.5f);
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            const sf::Vector2f collision_zone_pos = {
                note_x,
                static_cast<float>(static_cast<double>(collision_zone_y))
            };
            const sf::Vector2f collision_zone_size = {
                collizion_zone_width,
                static_cast<float>(static_cast<double>(collision_zone_height))
            };
            auto collision_zone_color = colors.normal_collision_zone;
            auto tap_note_color = [&](){
                if (use_quantization_colors) {
                    return quantization_colors.color_at_beat(long_note.get_time());
                } else {
                    return colors.normal_tap_note;
                }
            }();
            auto long_note_color = colors.normal_long_note;
            if (chart_state.chart.notes->is_colliding(long_note, timing, collision_zone)) {
                collision_zone_color = colors.conflicting_collision_zone;
                if (not use_quantization_colors) {
                    tap_note_color = colors.conflicting_tap_note;
                }
                long_note_color = colors.conflicting_long_note;
            }
            draw_rectangle(
                draw_list,
                origin + collision_zone_pos,
                collision_zone_size,
                {0.5f, 0.f},
                collision_zone_color
            );
            const auto long_note_rect_height = beats_to_pixels_proportional.transform(long_note.get_duration());
            const sf::Vector2f long_note_size = {
                long_note_rect_width,
                static_cast<float>(static_cast<double>(long_note_rect_height))
            };
            const sf::Vector2f note_pos = {note_x, note_y};
            draw_rectangle(
                draw_list,
                origin + note_pos,
                long_note_size,
                {0.5f, 0.f},
                long_note_color
            );
            draw_rectangle(
                draw_list,
                origin + note_pos,
                note_size,
                {0.5f, 0.5f},
                tap_note_color
            );
            if (chart_state.selected_stuff.notes.contains(long_note)) {
                draw_rectangle(
                    draw_list,
                    origin + note_pos,
                    selected_note_size,
                    {0.5f, 0.5f},
                    colors.selected_note_fill,
                    colors.selected_note_outline
                );
            }
        },
    };

    const auto first_visible_second = timing.time_at(first_beat_in_frame);
    const auto first_visible_collision_zone = timing.beats_at(first_visible_second - sf::milliseconds(500));
    const auto last_visible_second = timing.time_at(last_beat_in_frame);
    const auto last_visible_collision_zone = timing.beats_at(last_visible_second + sf::milliseconds(500));
    chart_state.chart.notes->in(
        first_visible_collision_zone,
        last_visible_collision_zone,
        [&](const better::Notes::iterator& it){
            it->second.visit(draw_note);
        }
    );

    if (chart_state.long_note_being_created.has_value()) {
        draw_note(
            make_long_note_dummy_for_linear_view(
                *chart_state.long_note_being_created,
                snap
            )
        );
    }

    // Draw the cursor
    const float cursor_width = timeline_width + 4.f;
    const float cursor_left = timeline_left - 2;
    const sf::Vector2f cursor_size = {cursor_width, 4.f};
    const sf::Vector2f cursor_pos = {cursor_left, cursor_y};
    draw_rectangle(
        draw_list,
        origin + cursor_pos,
        cursor_size,
        {0, 0.5},
        colors.cursor
    );

    // Draw the time selection
    const float selection_width = timeline_width;
    if (chart_state.time_selection.has_value()) {
        const auto pixel_interval = Interval{
            beats_to_pixels_absolute.transform(chart_state.time_selection->start),
            beats_to_pixels_absolute.transform(chart_state.time_selection->end)
        };
        if (pixel_interval.intersects({0, y})) {
            const sf::Vector2f selection_size = {
                selection_width,
                static_cast<float>(static_cast<double>(pixel_interval.width()))
            };
            const sf::Vector2f selection_pos = {
                timeline_left,
                static_cast<float>(static_cast<double>(pixel_interval.start))
            };
            draw_rectangle(
                draw_list,
                origin + selection_pos,
                selection_size,
                {0, 0},
                colors.tab_selection.fill,
                colors.tab_selection.border
            );
        }
    }

    
    const auto current_window = ImGui::GetCurrentWindow();

    // Don't start the selection rect if we start :
    //  - outside the contents of the window
    //  - over anything
    if (
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        and current_window->InnerClipRect.Contains(ImGui::GetMousePos())
        and not ImGui::IsAnyItemHovered()
        and ImGui::IsWindowFocused()
    ) {
        started_selection_inside_window = true;
    }

    if (started_selection_inside_window) {
        if (
            draw_selection_rect(
                draw_list,
                selection_rectangle.start,
                selection_rectangle.end,
                colors.selection_rect
            )
        ) {
            chart_state.selected_stuff.clear();
            // Select everything inside the selection rectangle
            const sf::Vector2f upper_left = {
                std::min(selection_rectangle.start.x, selection_rectangle.end.x),
                std::min(selection_rectangle.start.y, selection_rectangle.end.y),
            };
            const sf::Vector2f lower_right = {
                std::max(selection_rectangle.start.x, selection_rectangle.end.x),
                std::max(selection_rectangle.start.y, selection_rectangle.end.y),
            };
            const ImRect full_selection = {upper_left, lower_right};
            ImRect bpm_zone = {origin.x + bpm_events_left, -INFINITY, INFINITY, INFINITY};
            bpm_zone.ClipWith(current_window->InnerRect);
            if (full_selection.Overlaps(bpm_zone)) {
                const auto first_selected_beat = beats_to_pixels_absolute.backwards_transform(full_selection.Min.y - origin.y);
                const auto last_selected_beat = beats_to_pixels_absolute.backwards_transform(full_selection.Max.y - origin.y);
                timing.for_each_event_between(
                    first_selected_beat,
                    last_selected_beat,
                    [&](const auto& event){
                        chart_state.selected_stuff.bpm_events.insert(
                            {event.get_bpm(), event.get_beats()}
                        );
                    }
                );
            }
            selection_rectangle.reset();
            started_selection_inside_window = false;
        }
    }
}

void LinearView::set_zoom(int newZoom) {
    zoom = std::clamp(newZoom, -10, 10);
    reload_transforms();
}

void LinearView::display_settings() {
    if (ImGui::Begin("Vertical View Settings", &shouldDisplaySettings)) {
        if (ImGui::SliderInt("Zoom##Vertical View Settings", &zoom, -10, 10, "%d")) {
            set_zoom(zoom);
        }
        if (ImGui::BeginCombo("Mode##Vertical View Settings", mode_name().c_str())) {
            if (ImGui::Selectable(
                "Beats",
                std::holds_alternative<linear_view::mode::Beats>(mode)
            )) {
                mode = linear_view::mode::Beats{};
            }
            if (ImGui::Selectable(
                "Waveform",
                std::holds_alternative<linear_view::mode::Waveform>(mode)
            )) {
            }
            ImGui::EndCombo();
        }
        if (ImGui::CollapsingHeader("Notes##Vertical View Settings")) {
            ImGui::Checkbox("Colored Quantization", &use_quantization_colors);
            if (use_quantization_colors) {
                for (auto& [quant, color] : quantization_colors.palette) {
                    feis::ColorEdit4(
                        fmt::format(
                            "{}##Colored Quantization",
                            Toolbox::toOrdinal(quant*4)
                        ).c_str(),
                        color
                    );
                }
                feis::ColorEdit4("Other", quantization_colors.default_);
                if (ImGui::Button("Reset##Colored Quantization")) {
                    quantization_colors = linear_view::default_quantization_colors;
                }
            }
        }
        if (ImGui::CollapsingHeader("Lanes##Vertical View Settings")) {
            if (ImGui::BeginCombo("Order", lane_order_name().c_str())) {
                if (ImGui::Selectable(
                    "Default",
                    std::holds_alternative<linear_view::lane_order::Default>(lane_order)
                )) {
                    lane_order = linear_view::lane_order::Default{};
                }
                if (ImGui::Selectable(
                    "Vertical",
                    std::holds_alternative<linear_view::lane_order::Vertical>(lane_order)
                )) {
                    lane_order = linear_view::lane_order::Vertical{};
                }
                if (ImGui::Selectable(
                    "Custom",
                    std::holds_alternative<linear_view::lane_order::Custom>(lane_order)
                )) {
                    if (not std::holds_alternative<linear_view::lane_order::Custom>(lane_order)) {
                        lane_order = linear_view::lane_order::Custom{};
                    }
                }
                ImGui::EndCombo();
            }
            if (std::holds_alternative<linear_view::lane_order::Custom>(lane_order)) {
                auto& order = std::get<linear_view::lane_order::Custom>(lane_order);
                if (ImGui::InputText("Custom", &order.as_string)) {
                    order.cleanup_string();
                    order.update_from_string();
                };
                ImGui::Text("Preview :");
                LaneOrderPreview(order.lane_to_button);
            }
        }
        if (ImGui::CollapsingHeader("Colors##Vertical View Settings")) {
            if (ImGui::Button("Reset##Colors##Vertical View Settings")) {
                colors = linear_view::default_colors;
            }
            feis::ColorEdit4("Cursor", colors.cursor);
            feis::ColorEdit4("Measure Lines", colors.measure_line);
            feis::ColorEdit4("Measure Numbers", colors.measure_number);
            feis::ColorEdit4("Beat Lines", colors.beat_line);
            if (ImGui::TreeNode("Selection Rectangle")) {
                feis::ColorEdit4("Fill##Selection Rectangle Colors", colors.selection_rect.fill);
                feis::ColorEdit4("Border##Selection Rectangle Colors", colors.selection_rect.border);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("BPM Events##Color settings")) {
                feis::ColorEdit4("Text##BPM Event Color", colors.bpm_button.text);
                feis::ColorEdit4("Button##BPM Event Color", colors.bpm_button.button);
                feis::ColorEdit4("Hover##BPM Event Color", colors.bpm_button.hover);
                feis::ColorEdit4("Active##BPM Event Color", colors.bpm_button.active);
                feis::ColorEdit4("Border##BPM Event Color", colors.bpm_button.border);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Tab Selection##Color settings")) {
                feis::ColorEdit4("Fill##Tab Selection", colors.tab_selection.fill);
                feis::ColorEdit4("Border##Tab Selection", colors.tab_selection.border);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Notes##Color settings tree element")) {
                feis::ColorEdit4("Note##Color settings", colors.normal_tap_note);
                feis::ColorEdit4("Note (conflict)##Color settings", colors.conflicting_tap_note);
                feis::ColorEdit4("Collision Zone##Color settings", colors.normal_collision_zone);
                feis::ColorEdit4("Collision Zone (conflict)##Color settings", colors.conflicting_collision_zone);
                feis::ColorEdit4("Long Tail##Color settings", colors.normal_long_note);
                feis::ColorEdit4("Long Tail (conflict)##Color settings", colors.conflicting_long_note);
                feis::ColorEdit4("Selected Fill##Color settings", colors.selected_note_fill);
                feis::ColorEdit4("Selected Outline##Color settings", colors.selected_note_outline);
                ImGui::TreePop();
            }
        }
        if (ImGui::CollapsingHeader("Metrics##Vertical View Settings")) {
            if (ImGui::Button("Reset##Metrics##Vertical View Settings")) {
                sizes = linear_view::default_sizes;
            }
            ImGui::DragInt("Cursor Height", &sizes.cursor_height);
            ImGui::DragInt("Timeline Margin", &sizes.timeline_margin);
        }
    }
    ImGui::End();
}

std::string LinearView::mode_name() {
    const auto name = VariantVisitor {
        [](const linear_view::mode::Beats&){ return "Beats"; },
        [](const linear_view::mode::Waveform&){ return "Waveform"; },
    };
    return std::visit(name, mode);
}

void LinearView::reload_transforms() {
    beats_to_pixels_proportional = {
        Fraction{0},
        Fraction{1} / Fraction{time_factor()},
        Fraction{0},
        Fraction{100}
    };
}

std::string LinearView::lane_order_name() const {
    const auto name = VariantVisitor {
        [](linear_view::lane_order::Default) { return "Default"; },
        [](linear_view::lane_order::Vertical) { return "Vertical"; },
        [](linear_view::lane_order::Custom) { return "Custom"; },
    };
    return std::visit(name, lane_order);
}

std::optional<unsigned int> LinearView::button_to_lane(const better::Position& button) {
    const auto _button_to_lane = VariantVisitor {
        [button](const linear_view::lane_order::Default&){
            return static_cast<std::optional<unsigned int>>(button.index());
        },
        [button](const linear_view::lane_order::Vertical&){
            return static_cast<std::optional<unsigned int>>(button.get_y() + 4 * button.get_x());
        },
        [button](const linear_view::lane_order::Custom& c){
            const auto pair = c.button_to_lane.find(button.index());
            if (pair != c.button_to_lane.end()) {
                return static_cast<std::optional<unsigned int>>(pair->second);
            } else {
                return std::optional<unsigned int>{};
            }
        },
    };
    return std::visit(_button_to_lane, lane_order);
}

void draw_rectangle(
    ImDrawList* draw_list,
    const sf::Vector2f& pos,
    const sf::Vector2f& size,
    const sf::Vector2f& normalized_anchor,
    const sf::Color& fill,
    const std::optional<sf::Color>& outline
) {
    const auto real_pos = Toolbox::top_left_given_normalized_anchor(
        pos,
        size,
        normalized_anchor
    );
    // Fill
    draw_list->AddRectFilled(
        real_pos,
        real_pos + size,
        ImColor(ImVec4(fill))
    );
    // Outline
    if (outline) {
        draw_list->AddRect(
            real_pos - sf::Vector2f{1, 1},
            real_pos + size + sf::Vector2f{1, 1},
            ImColor(ImVec4(*outline))
        );
    }
}

bool BPMButton(
    const better::BPMEvent& event,
    bool selected,
    const sf::Vector2f& pos,
    const ButtonColors& colors
) {
    const auto bpm_text = event.get_bpm().format(".3f");
    const sf::Vector2f text_size = ImGui::CalcTextSize(bpm_text.c_str(), bpm_text.c_str()+bpm_text.size());
    const auto style = ImGui::GetStyle();
    sf::Vector2f button_size = ImGui::CalcItemSize(
        sf::Vector2f{0,0},
        text_size.x + style.FramePadding.x * 2.0f,
        text_size.y + style.FramePadding.y * 2.0f
    );
    const auto bpm_button_pos = Toolbox::bottom_left_given_normalized_anchor(
        pos,
        button_size,
        {0.f, 0.5f}
    );
    ImGui::SetCursorPos(bpm_button_pos);
    ImGui::PushID(&event);
    if (selected) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    }
    ImGui::PushStyleColor(ImGuiCol_Button, colors.button);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.active);
    ImGui::PushStyleColor(ImGuiCol_Text, colors.text);
    ImGui::PushStyleColor(ImGuiCol_Border, colors.border);
    const auto result = ImGui::Button(bpm_text.c_str());
    ImGui::PopStyleColor(5);
    if (selected) {
        ImGui::PopStyleVar();
    }
    ImGui::PopID();
    return result;
}

void cross(ImDrawList* draw_list, const sf::Vector2f& pos) {
    draw_list->AddLine(pos - sf::Vector2f{3, 0}, pos + sf::Vector2f{4, 0}, ImColor(sf::Color::White));
    draw_list->AddLine(pos - sf::Vector2f{0, 3}, pos + sf::Vector2f{0, 4}, ImColor(sf::Color::White));
}

// thanks rokups
// https://github.com/ocornut/imgui/issues/4883#issuecomment-1143414484
bool draw_selection_rect(
    ImDrawList* draw_list,
    sf::Vector2f& start_pos,
    sf::Vector2f& end_pos,
    const RectangleColors& colors,
    ImGuiMouseButton mouse_button
) {
    if (ImGui::IsMouseClicked(mouse_button)) {
        start_pos = ImGui::GetMousePos();
    }
    if (ImGui::IsMouseDown(mouse_button)) {
        end_pos = ImGui::GetMousePos();
        draw_rectangle(draw_list, start_pos, end_pos - start_pos, {0,0}, colors.fill, colors.border);
    }
    return ImGui::IsMouseReleased(mouse_button);
}

// (H: [0° -> 360°], S: 100%, L: 62.581%)
// in HSLuv https://www.hsluv.org/
static std::array<sf::Color, 16> rainbow = {{
    {255, 94, 137},
    {255, 103, 23},
    {210, 135, 0},
    {180, 149, 0},
    {151, 158, 0},
    {108, 168, 0},
    {0, 175, 78},
    {0, 172, 132},
    {0, 170, 157},
    {0, 168, 178},
    {0, 165, 204},
    {0, 157, 253},
    {152, 134, 255},
    {213, 103, 255},
    {255, 65, 235},
    {255, 84, 186},
}};

void LaneOrderPreview(const std::array<std::optional<unsigned int>, 16>& order) {
    const float scale = 16.f;
    const float arrow_x_pos = 4.5f;
    const auto origin = sf::Vector2f{ImGui::GetCursorPos()};
    const auto screen_origin = sf::Vector2f{ImGui::GetCursorScreenPos()};
    for (auto y = 0; y < 4; y++) {
        for (auto x = 0; x < 4; x++) {
            const auto index = x + 4*y;
            ImGui::SetCursorPos(origin + sf::Vector2f{static_cast<float>(x), static_cast<float>(y)} * scale);
            ImGui::TextColored(rainbow.at(index), "%s", linear_view::lane_order::letters.at(index).c_str());
            if (x != 3) {
                ImGui::SameLine();
            }
        }
    }
    ImGui::RenderArrow(
        ImGui::GetWindowDrawList(),
        screen_origin + sf::Vector2f{arrow_x_pos, 1.5f} * scale,
        ImColor(sf::Color::White),
        ImGuiDir_Right
    );
    for (std::size_t lane = 0; lane < order.size(); lane++) {
        ImGui::SetCursorPos(origin + sf::Vector2f{static_cast<float>(arrow_x_pos+2+lane), 1.5f} * scale);
        const auto optional_button = order.at(lane);
        if (optional_button) {
            const auto button = *optional_button % 16;
            ImGui::TextColored(rainbow.at(button), "%s", linear_view::lane_order::letters.at(button).c_str());
        } else {
            ImGui::TextDisabled("_");
        }
    }
    ImGui::SetCursorPos(origin);
    ImGui::Dummy(sf::Vector2f{23, 4}*scale);
}