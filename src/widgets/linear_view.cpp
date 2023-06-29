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
#include "linear_view_colors.hpp"
#include "waveform.hpp"
#include "widgets/lane_order.hpp"


const std::string font_file = "fonts/NotoSans-Medium.ttf";

void SelectionRectangle::reset() {
    start = {-1, -1};
    end = {-1, -1};
}

LinearView::LinearView(std::filesystem::path assets, config::Config& config_) :
    mode(config_.linear_view.mode),
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

void LinearView::draw(LinearView::DrawArgs& args) {
    const auto draw_function = VariantVisitor {
        [&](linear_view::mode::Beats) {this->draw_in_beats_mode(args);},
        [&](linear_view::mode::Waveform) {this->draw_in_waveform_mode(args);},
    };
    std::visit(draw_function, mode);
}

linear_view::ComputedSizes linear_view::compute_sizes(
    const sf::Vector2f& window_size,
    linear_view::Sizes& size_settings
) {
    ComputedSizes result;
    result.x = std::max(300, static_cast<int>(window_size.x));
    result.y = std::max(300, static_cast<int>(window_size.y));

    result.timeline_width = static_cast<float>(result.x) - static_cast<float>(size_settings.timeline_margin);
    result.timeline_left = static_cast<float>(size_settings.timeline_margin) / 2;
    result.timeline_right = result.timeline_left + result.timeline_width;

    // Just in case, clamp the beat cursor inside the window, with some margin
    result.cursor_y = std::clamp(static_cast<float>(size_settings.cursor_height), 25.f, static_cast<float>(result.y) - 25.f);
    
    result.bpm_events_left = result.timeline_right + 10;

    result.note_width = result.timeline_width / 16.f;
    result.collizion_zone_width = result.note_width - 2.f;
    result.long_note_rect_width = result.note_width * 0.75f;

    // Pre-size & center the shapes that can be // ??
    result.note_size = {result.note_width, 6.f};
    result.selected_note_size = {result.note_width + 2.f, 8.f};

    result.cursor_width = result.timeline_width + 4.f;
    result.cursor_left = result.timeline_left - 2;
    result.cursor_size = {result.cursor_width, 4.f};
    result.cursor_pos = {result.cursor_left, result.cursor_y};

    return result;
}

void LinearView::draw_in_beats_mode(LinearView::DrawArgs& args) {
    auto [
        draw_list,
        chart_state,
        waveform_cache,
        timing,
        current_beat,
        last_editable_beat,
        snap,
        window_size,
        origin
    ] = args;
    const auto computed_sizes = linear_view::compute_sizes(window_size, sizes);

    // Here we compute the range of visible beats from the size of the window
    // in pixels, we know by definition that the current beat is exactly at
    // cursor_y pixels and we use this fact to compute the rest
    const auto beats_before_cursor = beats_to_pixels_proportional.backwards_transform(computed_sizes.cursor_y);
    const auto beats_after_cursor = beats_to_pixels_proportional.backwards_transform(static_cast<float>(computed_sizes.y) - computed_sizes.cursor_y);
    const Fraction first_beat_in_frame = current_beat - beats_before_cursor;
    const Fraction last_beat_in_frame = current_beat + beats_after_cursor;
    AffineTransform<Fraction> beats_to_pixels_absolute{first_beat_in_frame, last_beat_in_frame, 0, computed_sizes.y};

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
        next_beat_line_y < computed_sizes.y and next_beat < last_editable_beat;
        next_beat_line_y = beats_to_pixels_absolute.transform(next_beat += 1)
    ) {
        const sf::Vector2f beat_line_start = {computed_sizes.timeline_left, static_cast<float>(static_cast<double>(next_beat_line_y))};
        const sf::Vector2f beat_line_end = beat_line_start + sf::Vector2f{computed_sizes.timeline_width, 0};
        if (next_beat % 4 == 0) {
            draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(colors.measure_line));
            const Fraction measure = next_beat / 4;
            const auto measure_string = fmt::format("{}", static_cast<std::int64_t>(measure));
            const sf::Vector2f text_size = ImGui::CalcTextSize(measure_string.c_str(), measure_string.c_str()+measure_string.size());
            const sf::Vector2f measure_text_pos = {computed_sizes.timeline_left - 10, static_cast<float>(static_cast<double>(next_beat_line_y))};
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

    // Draw the bpm changes
    timing.for_each_event_between(
        first_beat_in_frame,
        last_beat_in_frame,
        [&](const auto& event){
            const auto bpm_change_y = beats_to_pixels_absolute.transform(event.get_beats());
            if (bpm_change_y >= 0 and bpm_change_y <= computed_sizes.y) {
                const sf::Vector2f bpm_text_raw_pos = {computed_sizes.bpm_events_left, static_cast<float>(static_cast<double>(bpm_change_y))};
                const auto bpm_at_beat = better::BPMAtBeat{event.get_bpm(), event.get_beats()};
                const auto selected = args.chart_state.selected_stuff.bpm_events.contains(bpm_at_beat);
                if (BPMButton(event, selected, bpm_text_raw_pos, colors.bpm_button)) {
                    if (selected) {
                        args.chart_state.selected_stuff.bpm_events.erase(bpm_at_beat);
                    } else {
                        args.chart_state.selected_stuff.bpm_events.insert(bpm_at_beat);
                    }
                }
            }
        }
    );

    // Draw the notes
    auto draw_note = VariantVisitor {
        [&](const better::TapNote& tap_note){
            const auto opt_lane = button_to_lane(tap_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            const float note_x = computed_sizes.timeline_left + computed_sizes.note_width * (lane + 0.5f);
            const float note_y = static_cast<double>(beats_to_pixels_absolute.transform(tap_note.get_time()));
            const auto note_seconds = args.timing.time_at(tap_note.get_time());
            const auto first_colliding_beat = args.timing.beats_at(note_seconds - collision_zone * 0.5f);
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto last_colliding_beat = args.timing.beats_at(note_seconds + collision_zone * 0.5f);
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            draw_tap_note(
                args,
                computed_sizes,
                tap_note,
                {note_x, note_y},
                collision_zone,
                static_cast<float>(collision_zone_y),
                static_cast<float>(collision_zone_height)
            );
        },
        [&](const better::LongNote& long_note){
            const auto opt_lane = button_to_lane(long_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            float note_x = computed_sizes.timeline_left + computed_sizes.note_width * (lane + 0.5f);
            float note_y = static_cast<double>(beats_to_pixels_absolute.transform(long_note.get_time()));
            const auto note_start_seconds = args.timing.time_at(long_note.get_time());
            const auto first_colliding_beat = args.timing.beats_at(note_start_seconds - collision_zone * 0.5f);
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto note_end_seconds = args.timing.time_at(long_note.get_end());
            const auto last_colliding_beat = args.timing.beats_at(note_end_seconds + collision_zone * 0.5f);
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            const auto long_note_rect_height = beats_to_pixels_proportional.transform(long_note.get_duration());
            draw_long_note(
                args,
                computed_sizes,
                long_note,
                {note_x, note_y},
                static_cast<float>(long_note_rect_height),
                collision_zone,
                static_cast<float>(collision_zone_y),
                static_cast<float>(collision_zone_height)
            );
        },
    };

    const auto first_visible_second = timing.time_at(first_beat_in_frame);
    const auto last_visible_second = timing.time_at(last_beat_in_frame);
    draw_notes(
        first_visible_second,
        last_visible_second,
        chart_state,
        timing,
        draw_note
    );
    draw_long_note_dummy(chart_state, snap, draw_note);
    draw_cursor(draw_list, origin, computed_sizes);
    draw_time_selection(
        draw_list,
        origin,
        chart_state,
        computed_sizes,
        [&](const Fraction& beats) -> float {
            return static_cast<float>(beats_to_pixels_absolute.transform(beats));
        }
    );
    handle_mouse_selection(
        draw_list,
        origin,
        chart_state,
        timing,
        computed_sizes,
        [&](const float pixels){
            return beats_to_pixels_absolute.backwards_transform(pixels);
        }
    );
}


void LinearView::draw_in_waveform_mode(LinearView::DrawArgs& args) {
    auto [
        draw_list,
        chart_state,
        waveform_status,
        timing,
        current_beat,
        last_editable_beat,
        snap,
        window_size,
        origin
    ] = args;

    if (const auto message = std::visit(waveform::status_message, waveform_status)) {
        feis::CenteredText(*message);
        return;
    }
    if (not std::holds_alternative<waveform::status::Loaded>(waveform_status)) {
        feis::CenteredText("No waveform data ???");
        return;
    }
    const auto& waveform = std::get<waveform::status::Loaded>(waveform_status).waveform;
    if (waveform.channels_per_chunk_size.empty()) {
        feis::CenteredText("No data ???");
        return;
    }

    const auto computed_sizes = linear_view::compute_sizes(window_size, sizes);
    const auto chunk_sizes = waveform.zoom_to_params(zoom);
    const auto& channels = waveform.channels_per_chunk_size.at(chunk_sizes.reference);
    const auto window = ImGui::GetCurrentWindow();
    const auto work_rect = window->WorkRect;
    const float waveform_w_margin = 10.f;
    const float waveform_bounding_width = work_rect.GetWidth() / channels.size();
    const float waveform_width = waveform_bounding_width - waveform_w_margin;
    const AffineTransform<float> value_to_pixel_offset_from_waveform_center{
        std::numeric_limits<waveform::DataPoint::value_type>::min(),
        std::numeric_limits<waveform::DataPoint::value_type>::max(),
        -waveform_width / 2,
        waveform_width / 2
    };
    const auto current_time = timing.time_at(current_beat);
    const std::int64_t sample_at_cursor = time_to_samples(current_time, waveform.sample_rate, waveform.channel_count);
    const auto frac_chunk_at_cursor = static_cast<std::int64_t>(std::floor(
        static_cast<double>(sample_at_cursor) / waveform.channel_count / chunk_sizes.fractional
    ));
    const std::int64_t first_chunk = frac_chunk_at_cursor - static_cast<std::int64_t>(sizes.cursor_height);
    const std::int64_t end_chunk = first_chunk + static_cast<std::int64_t>(work_rect.GetHeight());
    const AffineTransform<float> seconds_to_pixels_proportional {
        0,
        1,
        0,
        static_cast<float>(waveform.sample_rate) / chunk_sizes.fractional
    };
    for (std::size_t channel_index = 0; channel_index < channels.size(); channel_index++) {
        const auto& data_points = channels[channel_index];
        const auto waveform_x_center = channel_index * waveform_bounding_width + waveform_bounding_width / 2;
        for (std::int64_t data_point_index = first_chunk; data_point_index < end_chunk; data_point_index++) {
            if (data_point_index < 0 or static_cast<std::size_t>(data_point_index) >= data_points.size()) {
                continue;
            }
            const float float_reference_chunk = data_point_index * chunk_sizes.frac_to_ref_ratio;
            const auto reference_chunk_before = std::clamp<std::int64_t>(
                std::floor(float_reference_chunk),
                0,
                data_points.size()
            );
            const auto reference_chunk_after = std::clamp<std::int64_t>(
                std::ceil(float_reference_chunk),
                0,
                data_points.size()
            );
            const float lerp_t = std::clamp<float>(float_reference_chunk - reference_chunk_before, 0, 1);
            const auto& data_point_before = data_points[reference_chunk_before];
            const auto& data_point_after = data_points[reference_chunk_after];
            const auto lerped_min = std::lerp(data_point_before.min, data_point_after.min, lerp_t);
            const auto lerped_max = std::lerp(data_point_before.max, data_point_after.max, lerp_t);
            const auto y = work_rect.Min.y + data_point_index - first_chunk;
            const auto x_offset_min = value_to_pixel_offset_from_waveform_center.transform(lerped_min);
            const auto x_offset_max = value_to_pixel_offset_from_waveform_center.transform(lerped_max);
            const auto x_min = work_rect.Min.x + waveform_x_center + x_offset_min;
            const auto x_max = work_rect.Min.x + waveform_x_center + x_offset_max;
            draw_list->AddLine({x_min, y}, {x_max, y}, ImColor(colors.waveform));
        }
    }

    const auto seconds_before_cursor = seconds_to_pixels_proportional.backwards_transform(computed_sizes.cursor_y);
    const auto seconds_after_cursor = seconds_to_pixels_proportional.backwards_transform(static_cast<float>(computed_sizes.y) - computed_sizes.cursor_y);
    const sf::Time first_visible_second = current_time - sf::seconds(seconds_before_cursor);
    const sf::Time last_visible_second = current_time + sf::seconds(seconds_after_cursor);
    const AffineTransform<float> seconds_to_pixels_absolute{
        first_visible_second.asSeconds(),
        last_visible_second.asSeconds(),
        0,
        static_cast<float>(computed_sizes.y)
    };

    const auto beats_to_absolute_pixels = [&](const Fraction& beat){
        return seconds_to_pixels_absolute.transform(args.timing.seconds_at(beat));
    };

    const Fraction first_visible_beat = std::max(Fraction{0}, timing.beats_at(first_visible_second));
    auto next_beat = [](const auto& first_beat) -> Fraction {
        if (first_beat % 1 == 0) {
            return first_beat;
        } else {
            return floor_fraction(first_beat) + 1;
        }
    }(first_visible_beat);

    // Draw the beat lines and numbers
    {
        Fraction next_beat_line_y = beats_to_absolute_pixels(next_beat);
        while (next_beat_line_y < computed_sizes.y and next_beat < last_editable_beat) {
            const sf::Vector2f beat_line_start = {computed_sizes.timeline_left, static_cast<float>(static_cast<double>(next_beat_line_y))};
            const sf::Vector2f beat_line_end = beat_line_start + sf::Vector2f{computed_sizes.timeline_width, 0};
            if (next_beat % 4 == 0) {
                draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(colors.measure_line));
                const Fraction measure = next_beat / 4;
                const auto measure_string = fmt::format("{}", static_cast<std::int64_t>(measure));
                const sf::Vector2f text_size = ImGui::CalcTextSize(measure_string.c_str(), measure_string.c_str()+measure_string.size());
                const sf::Vector2f measure_text_pos = {computed_sizes.timeline_left - 10, static_cast<float>(static_cast<double>(next_beat_line_y))};
                draw_list->AddText(
                    origin + measure_text_pos - sf::Vector2f{text_size.x, text_size.y * 0.5f},
                    ImColor(colors.measure_number),
                    measure_string.c_str(),
                    measure_string.c_str() + measure_string.size()
                );
            } else {
                draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(colors.beat_line));
            }
            next_beat += 1;
            next_beat_line_y = beats_to_absolute_pixels(next_beat);
        }
    }

    // Draw the bpm changes
    timing.for_each_event_between(
        first_visible_second,
        last_visible_second,
        [&](const auto& event){
            const auto bpm_change_y = beats_to_absolute_pixels(event.get_beats());
            if (bpm_change_y >= 0 and bpm_change_y <= computed_sizes.y) {
                const sf::Vector2f bpm_text_raw_pos = {computed_sizes.bpm_events_left, static_cast<float>(static_cast<double>(bpm_change_y))};
                const auto bpm_at_beat = better::BPMAtBeat{event.get_bpm(), event.get_beats()};
                const auto selected = args.chart_state.selected_stuff.bpm_events.contains(bpm_at_beat);
                if (BPMButton(event, selected, bpm_text_raw_pos, colors.bpm_button)) {
                    if (selected) {
                        args.chart_state.selected_stuff.bpm_events.erase(bpm_at_beat);
                    } else {
                        args.chart_state.selected_stuff.bpm_events.insert(bpm_at_beat);
                    }
                }
            }
        }
    );

    // Draw the notes
    auto draw_note = VariantVisitor {
        [&](const better::TapNote& tap_note){
            const auto opt_lane = button_to_lane(tap_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            const float note_x = computed_sizes.timeline_left + computed_sizes.note_width * (lane + 0.5f);
            const float note_y = beats_to_absolute_pixels(tap_note.get_time());
            const auto note_seconds = args.timing.time_at(tap_note.get_time());
            const auto collision_start_time = note_seconds - collision_zone * 0.5f;
            const auto collision_zone_y = seconds_to_pixels_absolute.transform(collision_start_time.asSeconds());
            const auto collision_end_time = note_seconds + collision_zone * 0.5f;
            const auto collision_zone_height = seconds_to_pixels_proportional.transform(
                (collision_end_time - collision_start_time).asSeconds()
            );
            draw_tap_note(
                args,
                computed_sizes,
                tap_note,
                {note_x, note_y},
                collision_zone,
                collision_zone_y,
                collision_zone_height
            );
        },
        [&](const better::LongNote& long_note){
            const auto opt_lane = button_to_lane(long_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            const float note_x = computed_sizes.timeline_left + computed_sizes.note_width * (lane + 0.5f);
            const float note_y = beats_to_absolute_pixels(long_note.get_time());
            const auto note_start_seconds = args.timing.time_at(long_note.get_time());
            const auto collision_start_time = note_start_seconds - collision_zone * 0.5f;
            const auto collision_zone_y = seconds_to_pixels_absolute.transform(collision_start_time.asSeconds());
            const auto note_end_seconds = args.timing.time_at(long_note.get_end());
            const auto collision_end_time = note_end_seconds + collision_zone * 0.5f;
            const auto collision_zone_height = seconds_to_pixels_proportional.transform(
                (collision_end_time - collision_start_time).asSeconds()
            );
            const auto long_note_rect_height = seconds_to_pixels_proportional.transform(
                (note_end_seconds - note_start_seconds).asSeconds()
            );
            draw_long_note(
                args,
                computed_sizes,
                long_note,
                {note_x, note_y},
                long_note_rect_height,
                collision_zone,
                collision_zone_y,
                collision_zone_height
            );
        },
    };

    draw_notes(
        first_visible_second,
        last_visible_second,
        chart_state,
        timing,
        draw_note
    );
    draw_long_note_dummy(chart_state, snap, draw_note);
    draw_cursor(draw_list, origin, computed_sizes);
    draw_time_selection(draw_list, origin, chart_state, computed_sizes, beats_to_absolute_pixels);
    handle_mouse_selection(
        draw_list,
        origin,
        chart_state,
        timing,
        computed_sizes,
        [&](const float pixels){
            return args.timing.beats_at(
                seconds_to_pixels_absolute.backwards_transform(pixels)
            );
        }
    );
}


void LinearView::draw_tap_note(
    LinearView::DrawArgs& args,
    const linear_view::ComputedSizes& computed_sizes,
    const better::TapNote& tap_note,
    const sf::Vector2f note_pos,
    const sf::Time collision_zone,
    const float collision_zone_y,
    const float collision_zone_height
) {
    const sf::Vector2f collision_zone_pos = {
        note_pos.x,
        collision_zone_y
    };
    const sf::Vector2f collizion_zone_size = {
        computed_sizes.collizion_zone_width,
        collision_zone_height
    };
    const auto collision_zone_color = [&](){
        if (args.chart_state.chart.notes->is_colliding(tap_note, args.timing, collision_zone)) {
            return colors.conflicting_collision_zone;
        } else {    
            return colors.normal_collision_zone;
        }
    }();
    const auto tap_note_color = [&](){
        if (use_quantization_colors) {
            return quantization_colors.color_at_beat(tap_note.get_time());
        } else if (args.chart_state.chart.notes->is_colliding(tap_note, args.timing, collision_zone)) {
            return colors.conflicting_tap_note;
        } else {
            return colors.normal_tap_note;
        }
    }();
    draw_rectangle(
        args.draw_list,
        args.origin + collision_zone_pos,
        collizion_zone_size,
        {0.5f, 0.f},
        collision_zone_color
    );
    draw_rectangle(
        args.draw_list,
        args.origin + note_pos,
        computed_sizes.note_size,
        {0.5f, 0.5f},
        tap_note_color
    );
    if (args.chart_state.selected_stuff.notes.contains(tap_note)) {
        draw_rectangle(
            args.draw_list,
            args.origin + note_pos,
            computed_sizes.selected_note_size,
            {0.5f, 0.5f},
            colors.selected_note_fill,
            colors.selected_note_outline
        );
    }
}

void LinearView::draw_long_note(
    LinearView::DrawArgs& args,
    const linear_view::ComputedSizes& computed_sizes,
    const better::LongNote& long_note,
    const sf::Vector2f note_pos,
    const float long_note_rect_height,
    const sf::Time collision_zone,
    const float collision_zone_y,
    const float collision_zone_height
) {
    const sf::Vector2f collision_zone_pos = {
        note_pos.x,
        static_cast<float>(static_cast<double>(collision_zone_y))
    };
    const sf::Vector2f collision_zone_size = {
        computed_sizes.collizion_zone_width,
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
    if (args.chart_state.chart.notes->is_colliding(long_note, args.timing, collision_zone)) {
        collision_zone_color = colors.conflicting_collision_zone;
        if (not use_quantization_colors) {
            tap_note_color = colors.conflicting_tap_note;
        }
        long_note_color = colors.conflicting_long_note;
    }
    draw_rectangle(
        args.draw_list,
        args.origin + collision_zone_pos,
        collision_zone_size,
        {0.5f, 0.f},
        collision_zone_color
    );
    const sf::Vector2f long_note_size = {
        computed_sizes.long_note_rect_width,
        long_note_rect_height
    };
    draw_rectangle(
        args.draw_list,
        args.origin + note_pos,
        long_note_size,
        {0.5f, 0.f},
        long_note_color
    );
    draw_rectangle(
        args.draw_list,
        args.origin + note_pos,
        computed_sizes.note_size,
        {0.5f, 0.5f},
        tap_note_color
    );
    if (args.chart_state.selected_stuff.notes.contains(long_note)) {
        draw_rectangle(
            args.draw_list,
            args.origin + note_pos,
            computed_sizes.selected_note_size,
            {0.5f, 0.5f},
            colors.selected_note_fill,
            colors.selected_note_outline
        );
    }
}

void LinearView::draw_cursor(
    ImDrawList* draw_list,
    const sf::Vector2f& origin,
    const linear_view::ComputedSizes& computed_sizes
) {
    draw_rectangle(
        draw_list,
        origin + computed_sizes.cursor_pos,
        computed_sizes.cursor_size,
        {0, 0.5},
        colors.cursor
    );
}

void LinearView::draw_time_selection(
    ImDrawList* draw_list,
    const sf::Vector2f& origin,
    const ChartState& chart_state,
    const linear_view::ComputedSizes& computed_sizes,
    const std::function<float(const Fraction&)> beats_to_absolute_pixels
) {
    const float selection_width = computed_sizes.timeline_width;
    if (chart_state.time_selection.has_value()) {
        const auto pixel_interval = Interval{
            beats_to_absolute_pixels(chart_state.time_selection->start),
            beats_to_absolute_pixels(chart_state.time_selection->end)
        };
        if (pixel_interval.intersects({0, static_cast<float>(computed_sizes.y)})) {
            const sf::Vector2f selection_size = {
                selection_width,
                static_cast<float>(pixel_interval.width())
            };
            const sf::Vector2f selection_pos = {
                computed_sizes.timeline_left,
                static_cast<float>(pixel_interval.start)
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
}

void LinearView::handle_mouse_selection(
    ImDrawList* draw_list,
    const sf::Vector2f& origin,
    ChartState& chart_state,
    const better::Timing& timing,
    const linear_view::ComputedSizes& computed_sizes,
    const std::function<Fraction(float)> absolute_pixels_to_beats
) {
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
            ImRect bpm_zone = {origin.x + computed_sizes.bpm_events_left, -INFINITY, INFINITY, INFINITY};
            bpm_zone.ClipWith(current_window->InnerRect);
            if (full_selection.Overlaps(bpm_zone)) {
                const auto first_selected_beat = absolute_pixels_to_beats(full_selection.Min.y - origin.y);
                const auto last_selected_beat = absolute_pixels_to_beats(full_selection.Max.y - origin.y);
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
    if (ImGui::Begin("Linear View Settings", &shouldDisplaySettings)) {
        if (ImGui::SliderInt("Zoom##Linear View Settings", &zoom, -10, 10, "%d")) {
            set_zoom(zoom);
        }
        if (ImGui::BeginCombo("Mode##Linear View Settings", mode_name().c_str())) {
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
                mode = linear_view::mode::Waveform{};
            }
            ImGui::EndCombo();
        }
        if (ImGui::CollapsingHeader("Notes##Linear View Settings")) {
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
        if (ImGui::CollapsingHeader("Lanes##Linear View Settings")) {
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
        if (ImGui::CollapsingHeader("Colors##Linear View Settings")) {
            if (ImGui::Button("Reset##Colors##Linear View Settings")) {
                colors = linear_view::default_colors;
            }
            feis::ColorEdit4("Cursor", colors.cursor);
            feis::ColorEdit4("Measure Lines", colors.measure_line);
            feis::ColorEdit4("Measure Numbers", colors.measure_number);
            feis::ColorEdit4("Beat Lines", colors.beat_line);
            feis::ColorEdit4("Waveform", colors.waveform);
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
        if (ImGui::CollapsingHeader("Metrics##Linear View Settings")) {
            if (ImGui::Button("Reset##Metrics##Linear View Settings")) {
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