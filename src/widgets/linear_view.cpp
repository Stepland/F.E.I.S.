#include "linear_view.hpp"

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
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>
#include <SFML/System/Time.hpp>

#include "../special_numeric_types.hpp"
#include "../toolbox.hpp"
#include "../chart_state.hpp"
#include "../long_note_dummy.hpp"
#include "../variant_visitor.hpp"
#include "better_note.hpp"
#include "src/better_timing.hpp"
#include "src/imgui_extras.hpp"

const std::string font_file = "fonts/NotoSans-Medium.ttf";

void SelectionRectangle::reset() {
    start = {-1, -1};
    end = {-1, -1};
}

LinearView::LinearView(std::filesystem::path assets) :
    beats_to_pixels_proportional(0, 1, 0, 100),
    lane_order(LaneOrderPresets::Default{})
{}

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
                if (BPMButton(event, selected, bpm_text_raw_pos, bpm_button_colors)) {
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
            if (chart_state.selected_stuff.notes.contains(tap_note)) {
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
            const auto opt_lane = button_to_lane(long_note.get_position());
            if (not opt_lane) {
                return;
            }
            const auto lane = *opt_lane;
            float note_x = timeline_left + note_width * (lane + 0.5f);
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
            if (chart_state.selected_stuff.notes.contains(long_note)) {
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
                tab_selection_colors.fill,
                tab_selection_colors.border
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
                selection_rect_colors
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

const std::array<std::string, 16> letters = {
    "1","2","3","4",
    "5","6","7","8",
    "9","a","b","c",
    "d","e","f","g"
};

void LinearView::display_settings() {
    if (ImGui::Begin("Linear View Settings", &shouldDisplaySettings)) {
        if (ImGui::CollapsingHeader("Lanes##Linear View Settings")) {
            if (ImGui::BeginCombo("Order", lane_order_name().c_str())) {
                if (ImGui::Selectable(
                    "Default",
                    std::holds_alternative<LaneOrderPresets::Default>(lane_order)
                )) {
                    lane_order = LaneOrderPresets::Default{};
                }
                if (ImGui::Selectable(
                    "Vertical",
                    std::holds_alternative<LaneOrderPresets::Vertical>(lane_order)
                )) {
                    lane_order = LaneOrderPresets::Vertical{};
                }
                if (ImGui::Selectable(
                    "Custom",
                    std::holds_alternative<CustomLaneOrder>(lane_order)
                )) {
                    if (not std::holds_alternative<CustomLaneOrder>(lane_order)) {
                        lane_order = CustomLaneOrder{};
                    }
                }
                ImGui::EndCombo();
            }
            if (std::holds_alternative<CustomLaneOrder>(lane_order)) {
                auto& order = std::get<CustomLaneOrder>(lane_order);
                if (ImGui::InputText("Custom", &order.as_string)) {
                    order.cleanup_string();
                    order.update_from_string();
                };
                ImGui::Text("Preview :");
                LaneOrderPreview(order.lane_to_button);
            }
        }
        if (ImGui::CollapsingHeader("Colors##Linear View Settings")) {
            feis::ColorEdit4("Cursor", cursor_color);
            feis::ColorEdit4("Measure Lines", measure_lines_color);
            feis::ColorEdit4("Measure Numbers", measure_numbers_color);
            feis::ColorEdit4("Beat Lines", beat_lines_color);
            if (ImGui::TreeNode("Selection Rectangle")) {
                feis::ColorEdit4("Fill##Selection Rectangle Colors", selection_rect_colors.fill);
                feis::ColorEdit4("Border##Selection Rectangle Colors", selection_rect_colors.border);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("BPM Events##Color settings")) {
                feis::ColorEdit4("Text##BPM Event Color", bpm_button_colors.text);
                feis::ColorEdit4("Button##BPM Event Color", bpm_button_colors.button);
                feis::ColorEdit4("Hover##BPM Event Color", bpm_button_colors.hover);
                feis::ColorEdit4("Active##BPM Event Color", bpm_button_colors.active);
                feis::ColorEdit4("Border##BPM Event Color", bpm_button_colors.border);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Tab Selection##Color settings")) {
                feis::ColorEdit4("Fill##Tab Selection", tab_selection_colors.fill);
                feis::ColorEdit4("Border##Tab Selection", tab_selection_colors.border);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Notes##Color settings tree element")) {
                feis::ColorEdit4("Note##Color settings", normal_tap_note_color);
                feis::ColorEdit4("Note (conflict)##Color settings", conflicting_tap_note_color);
                feis::ColorEdit4("Collision Zone##Color settings", normal_collision_zone_color);
                feis::ColorEdit4("Collision Zone (conflict)##Color settings", conflicting_collision_zone_color);
                feis::ColorEdit4("Long Tail##Color settings", normal_long_note_color);
                feis::ColorEdit4("Long Tail (conflict)##Color settings", conflicting_long_note_color);
                feis::ColorEdit4("Selected Fill##Color settings", selected_note_fill);
                feis::ColorEdit4("Selected Outline##Color settings", selected_note_outline);
                ImGui::TreePop();
            }
        }
        if (ImGui::CollapsingHeader("Metrics##Linear View Settings")) {
            ImGui::DragInt("Cursor Height", &cursor_height);
            ImGui::DragInt("Timeline Margin", &timeline_margin);
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

std::string LinearView::lane_order_name() {
    const auto name = VariantVisitor {
        [](LaneOrderPresets::Default) { return "Default"; },
        [](LaneOrderPresets::Vertical) { return "Vertical"; },
        [](CustomLaneOrder) { return "Custom"; },
    };
    return std::visit(name, lane_order);
}

std::optional<unsigned int> LinearView::button_to_lane(const better::Position& button) {
    const auto _button_to_lane = VariantVisitor {
        [button](const LaneOrderPresets::Default&){
            return static_cast<std::optional<unsigned int>>(button.index());
        },
        [button](const LaneOrderPresets::Vertical&){
            return static_cast<std::optional<unsigned int>>(button.get_y() + 4 * button.get_x());
        },
        [button](const CustomLaneOrder& c){
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
            ImGui::TextColored(rainbow.at(index), "%s", letters.at(index).c_str());
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
            ImGui::TextColored(rainbow.at(button), "%s", letters.at(button).c_str());
        } else {
            ImGui::TextDisabled("_");
        }
    }
    ImGui::SetCursorPos(origin);
    ImGui::Dummy(sf::Vector2f{23, 4}*scale);
}

LinearView::CustomLaneOrder::CustomLaneOrder() :
    lane_to_button({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15})
{
    update_from_array();
}

void LinearView::CustomLaneOrder::update_from_array() {
    std::stringstream ss;
    button_to_lane.clear();
    for (std::size_t lane = 0; lane < lane_to_button.size(); lane++) {
        const auto button = lane_to_button.at(lane);
        if (button) {
            ss << letters.at(*button);
            button_to_lane[*button] = lane;
        } else {
            ss << "_";
        }
    }
    as_string = ss.str();
}

const std::map<char, unsigned int> letter_to_index = {
    {'1', 0}, {'2', 1}, {'3', 2}, {'4', 3},
    {'5', 4}, {'6', 5}, {'7', 6}, {'8', 7},
    {'9', 8}, {'a', 9}, {'b', 10}, {'c', 11},
    {'d', 12}, {'e', 13}, {'f', 14}, {'g', 15},
};

void LinearView::CustomLaneOrder::cleanup_string() {
    as_string.resize(16);
    for (auto& c : as_string) {
        if (not letter_to_index.contains(c)) {
            c = '_';
        }
    }
}

void LinearView::CustomLaneOrder::update_from_string() {
    lane_to_button = {{
        {}, {}, {}, {},
        {}, {}, {}, {},
        {}, {}, {}, {},
        {}, {}, {}, {}
    }};
    button_to_lane.clear();
    const auto upper_bound = std::min(16UL, as_string.length());
    for (std::size_t lane = 0; lane < upper_bound; lane++) {
        const auto letter = as_string.at(lane);
        const auto pair = letter_to_index.find(letter);
        if (pair != letter_to_index.end()) {
            const auto button = pair->second;
            lane_to_button[lane] = button;
            button_to_lane[button] = lane;
        }
    }
}