#include "linear_view.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <functional>
#include <iostream>
#include <variant>

#include <fmt/core.h>
#include <imgui.h>
#include <SFML/System/Time.hpp>

#include "../special_numeric_types.hpp"
#include "../toolbox.hpp"
#include "../chart_state.hpp"
#include "../long_note_dummy.hpp"
#include "../variant_visitor.hpp"
#include "imgui_internal.h"
#include "src/imgui_extras.hpp"

const std::string font_file = "fonts/NotoSans-Medium.ttf";

LinearView::LinearView(std::filesystem::path assets) :
    beats_to_pixels_proportional(0, 1, 0, 100)
{}

void LinearView::draw(
    ImDrawList* draw_list,
    const ChartState& chart_state,
    const better::Timing& timing,
    const Fraction& current_beat,
    const Fraction& last_editable_beat,
    const Fraction& snap,
    const sf::Vector2f& size,
    const sf::Vector2f& origin
) {
    int x = std::max(300, static_cast<int>(size.x));
    int y = std::max(300, static_cast<int>(size.y));

    const float timeline_width = static_cast<float>(x) - static_cast<float>(timeline_margin);
    const float timeline_left = static_cast<float>(timeline_margin) / 2;
    const float timeline_right = timeline_left + timeline_width;

    // Just in case, clamp the beat cursor inside the window, with some margin
    const float cursor_y = std::clamp(static_cast<float>(cursor_height), 25.f, static_cast<float>(y) - 25.f);

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
            draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(measure_lines_color));
            const Fraction measure = next_beat / 4;
            const auto measure_string = fmt::format("{}", static_cast<std::int64_t>(measure));
            const sf::Vector2f text_size = ImGui::CalcTextSize(measure_string.c_str(), measure_string.c_str()+measure_string.size());
            const sf::Vector2f measure_text_pos = {timeline_left - 10, static_cast<float>(static_cast<double>(next_beat_line_y))};
            draw_list->AddText(
                origin + measure_text_pos - sf::Vector2f{text_size.x, text_size.y * 0.5f},
                ImColor(measure_numbers_color),
                measure_string.c_str(),
                measure_string.c_str() + measure_string.size()
            );
        } else {
            draw_list->AddLine(beat_line_start + origin, beat_line_end + origin, ImColor(beat_lines_color));
        }
    }

    // Draw the bpm changes
    const auto first_visible_bpm_change = timing.get_events_by_beats().lower_bound(
        {first_beat_in_frame, 0, 1}
    );
    const auto one_past_last_visible_bpm_change = timing.get_events_by_beats().upper_bound(
        {last_beat_in_frame, 0, 1}
    );
    for (
        auto it = first_visible_bpm_change;
        it != one_past_last_visible_bpm_change;
        ++it
    ) {
        const auto bpm_change_y = beats_to_pixels_absolute.transform(it->get_beats());
        if (bpm_change_y >= 0 and bpm_change_y <= y) {
            const auto bpm_text = it->get_bpm().format(".3f");
            const sf::Vector2f text_size = ImGui::CalcTextSize(bpm_text.c_str(), bpm_text.c_str()+bpm_text.size());
            const auto style = ImGui::GetStyle();
            sf::Vector2f button_size = ImGui::CalcItemSize(
                sf::Vector2f{0,0},
                text_size.x + style.FramePadding.x * 2.0f,
                text_size.y + style.FramePadding.y * 2.0f
            );
            const sf::Vector2f bpm_text_raw_pos = {timeline_right + 10, static_cast<float>(static_cast<double>(bpm_change_y))};
            const auto bpm_button_pos = Toolbox::bottom_left_given_normalized_anchor(
                bpm_text_raw_pos,
                button_size,
                {0.f, 0.5f}
            );
            ImGui::SetCursorPos(bpm_button_pos);
            ImGui::PushID(&*it);
            ImGui::PushStyleColor(ImGuiCol_Button, sf::Color::Transparent);
            ImGui::PushStyleColor(ImGuiCol_Text, bpm_text_color);
            ImGui::ButtonEx(bpm_text.c_str(), {0,0}, ImGuiButtonFlags_AlignTextBaseLine);
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
    }

    float note_width = timeline_width / 16.f;
    float collizion_zone_width = note_width - 2.f;
    float long_note_rect_width = note_width * 0.75f;

    // Pre-size & center the shapes that can be
    const sf::Vector2f note_size = {note_width, 6.f};

    const sf::Vector2f selected_note_size = {note_width + 2.f, 8.f};

    // Draw the notes
    auto draw_note = VariantVisitor {
        [&](const better::TapNote& tap_note){
            float note_x = timeline_left + note_width * (tap_note.get_position().index() + 0.5f);
            float note_y = static_cast<double>(beats_to_pixels_absolute.transform(tap_note.get_time()));
            const auto note_seconds = timing.time_at(tap_note.get_time());
            const auto first_colliding_beat = timing.beats_at(note_seconds - sf::milliseconds(500));
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto last_colliding_beat = timing.beats_at(note_seconds + sf::milliseconds(500));
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            const sf::Vector2f collision_zone_pos = {
                note_x,
                static_cast<float>(static_cast<double>(collision_zone_y))
            };
            const sf::Vector2f collizion_zone_size = {
                collizion_zone_width,
                static_cast<float>(static_cast<double>(collision_zone_height))
            };
            auto collision_zone_color = normal_collision_zone_color;
            auto tap_note_color = normal_tap_note_color;
            if (chart_state.chart.notes->is_colliding(tap_note, timing)) {
                collision_zone_color = conflicting_collision_zone_color;
                tap_note_color = conflicting_tap_note_color;
            }
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
            if (chart_state.selected_notes.contains(tap_note)) {
                draw_rectangle(
                    draw_list,
                    origin + note_pos,
                    selected_note_size,
                    {0.5f, 0.5f},
                    selected_note_fill,
                    selected_note_outline
                );
            }
        },
        [&](const better::LongNote& long_note){
            float note_x = timeline_left + note_width * (long_note.get_position().index() + 0.5f);
            float note_y = static_cast<double>(beats_to_pixels_absolute.transform(long_note.get_time()));
            const auto note_start_seconds = timing.time_at(long_note.get_time());
            const auto first_colliding_beat = timing.beats_at(note_start_seconds - sf::milliseconds(500));
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto note_end_seconds = timing.time_at(long_note.get_end());
            const auto last_colliding_beat = timing.beats_at(note_end_seconds + sf::milliseconds(500));
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            const sf::Vector2f collision_zone_pos = {
                note_x,
                static_cast<float>(static_cast<double>(collision_zone_y))
            };
            const sf::Vector2f collision_zone_size = {
                collizion_zone_width,
                static_cast<float>(static_cast<double>(collision_zone_height))
            };
            auto collision_zone_color = normal_collision_zone_color;
            auto tap_note_color = normal_tap_note_color;
            auto long_note_color = normal_long_note_color;
            if (chart_state.chart.notes->is_colliding(long_note, timing)) {
                collision_zone_color = conflicting_collision_zone_color;
                tap_note_color = conflicting_tap_note_color;
                long_note_color = conflicting_long_note_color;
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
            if (chart_state.selected_notes.contains(long_note)) {
                draw_rectangle(
                    draw_list,
                    origin + note_pos,
                    selected_note_size,
                    {0.5f, 0.5f},
                    selected_note_fill,
                    selected_note_outline
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
            make_linear_view_long_note_dummy(
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
        cursor_color
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
                tab_selection_fill,
                tab_selection_outline
            );
        }
    }
}

void LinearView::set_zoom(int newZoom) {
    zoom = std::clamp(newZoom, -10, 10);
    reload_transforms();
}

void LinearView::display_settings() {
    if (ImGui::Begin("Linear View Settings", &shouldDisplaySettings, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::TreeNodeEx("Palette##Linear View Settings",ImGuiTreeNodeFlags_DefaultOpen)) {
            feis::ColorEdit4("BPM Text", bpm_text_color);
            feis::ColorEdit4("Cursor", cursor_color);
            feis::ColorEdit4("Tab Selection Fill", tab_selection_fill);
            feis::ColorEdit4("Tab Selection Outline", tab_selection_outline);
            feis::ColorEdit4("Note (normal)", normal_tap_note_color);
            feis::ColorEdit4("Note (conflicting)", conflicting_tap_note_color);
            feis::ColorEdit4("Note Collision Zone (normal)", normal_collision_zone_color);
            feis::ColorEdit4("Note Collision Zone (conflicting)", conflicting_collision_zone_color);
            feis::ColorEdit4("Long Note Tail (normal)", normal_long_note_color);
            feis::ColorEdit4("Long Note Tail (conflicting)", conflicting_long_note_color);
            feis::ColorEdit4("Selected Note Fill", selected_note_fill);
            feis::ColorEdit4("Selected Note Outline", selected_note_outline);
            feis::ColorEdit4("Measure Lines", measure_lines_color);
            feis::ColorEdit4("Measure Numbers", measure_numbers_color);
            feis::ColorEdit4("Beat Lines", beat_lines_color);
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Metrics##Linear View Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragInt("Cursor Height", &cursor_height);
            ImGui::DragInt("Timeline Margin", &timeline_margin);
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void LinearView::reload_transforms() {
    beats_to_pixels_proportional = {
        Fraction{0},
        Fraction{1} / Fraction{time_factor()},
        Fraction{0},
        Fraction{100}
    };
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

void cross(ImDrawList* draw_list, const sf::Vector2f& pos) {
    draw_list->AddLine(pos - sf::Vector2f{3, 0}, pos + sf::Vector2f{4, 0}, ImColor(sf::Color::White));
    draw_list->AddLine(pos - sf::Vector2f{0, 3}, pos + sf::Vector2f{0, 4}, ImColor(sf::Color::White));
}
