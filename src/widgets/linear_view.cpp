#include "linear_view.hpp"

#include <functional>
#include <iostream>
#include <variant>

#include <SFML/System/Time.hpp>

#include "../special_numeric_types.hpp"
#include "../toolbox.hpp"
#include "../chart_state.hpp"
#include "../variant_visitor.hpp"

const std::string font_file = "fonts/NotoSans-Medium.ttf";

LinearView::LinearView(std::filesystem::path assets) :
    beats_to_pixels_proportional(0, 1, 0, 100),
    font_path(assets / font_file)
{
    if (!beat_number_font.loadFromFile(font_path)) {
        std::cerr << "Unable to load " << font_path;
        throw std::runtime_error("Unable to load " + font_path.string());
    }

    cursor.setFillColor(sf::Color(66, 150, 250, 200));
    cursor.setOrigin(0.f, 2.f);
    cursor.setPosition({48.f, cursor_y});

    selection.setFillColor(sf::Color(153, 255, 153, 92));
    selection.setOutlineColor(sf::Color(153, 255, 153, 189));
    selection.setOutlineThickness(1.f);

    tap_note_rect.setFillColor(sf::Color(255, 213, 0, 255));

    note_selected.setFillColor(sf::Color(255, 255, 255, 200));
    note_selected.setOutlineThickness(1.f);

    note_collision_zone.setFillColor(sf::Color(230, 179, 0, 127));

    long_note_rect.setFillColor(sf::Color(255, 90, 0, 223));
}

void LinearView::resize(unsigned int width, unsigned int height) {
    if (view.getSize() != sf::Vector2u(width, height)) {
        if (!view.create(width, height)) {
            std::cerr << "Unable to resize Playfield's longNoteLayer";
            throw std::runtime_error(
                "Unable to resize Playfield's longNoteLayer");
        }
        view.setSmooth(true);
    }
    view.clear(sf::Color::Transparent);
}

void LinearView::update(
    const ChartState& chart_state,
    const sf::Time& playback_position,
    const ImVec2& size
) {
    int x = std::max(140, static_cast<int>(size.x));
    int y = std::max(140, static_cast<int>(size.y));

    resize(static_cast<unsigned int>(x), static_cast<unsigned int>(y));

    // Just in case, clamp the beat cursor inside the window, with some margin
    cursor_y = std::clamp(cursor_y, 25.f, static_cast<float>(y) - 25.f);
    cursor.setPosition({48.f, cursor_y});

    // Here we compute the range of visible beats from the size of the window
    // in pixels, we know by definition that the current beat is exactly at
    // cursor_y pixels and we use this fact to compute the rest
    const auto beats_before_cursor = beats_to_pixels_proportional.backwards_transform(cursor_y);
    const auto beats_after_cursor = beats_to_pixels_proportional.backwards_transform(static_cast<float>(y) - cursor_y);
    const auto current_beat = chart_state.chart.timing.beats_at(playback_position);
    Fraction first_visible_beat = current_beat - beats_before_cursor;
    Fraction last_visible_beat = current_beat + beats_after_cursor;
    AffineTransform<Fraction> beats_to_pixels_absolute{first_visible_beat, last_visible_beat, 0, y};

    // Draw the beat lines and numbers
    auto next_beat = [&](const auto& first_beat) -> Fraction {
        if (first_beat % 1 == 0) {
            return first_beat;
        } else {
            return floor_fraction(first_beat) + 1;
        }
    }(first_visible_beat);

    auto next_beat_line_y = beats_to_pixels_absolute.backwards_transform(next_beat);

    float timeline_width = static_cast<float>(x) - 80.f;
    float timeline_x = 50.f;

    sf::RectangleShape beat_line(sf::Vector2f(timeline_width, 1.f));

    sf::Text beat_number;
    beat_number.setFont(beat_number_font);
    beat_number.setCharacterSize(15);
    beat_number.setFillColor(sf::Color::White);
    std::stringstream ss;

    while (next_beat_line_y < y) {
        if (next_beat % 4 == 0) {
            beat_line.setFillColor(sf::Color::White);
            beat_line.setPosition({timeline_x, static_cast<float>(next_beat_line_y)});
            view.draw(beat_line);

            ss.str(std::string());
            ss << static_cast<int>(next_beat / 4);
            beat_number.setString(ss.str());
            sf::FloatRect textRect = beat_number.getLocalBounds();
            beat_number.setOrigin(
                textRect.left + textRect.width,
                textRect.top + textRect.height / 2.f);
            beat_number.setPosition({40.f, static_cast<float>(next_beat_line_y)});
            view.draw(beat_number);

        } else {
            beat_line.setFillColor(sf::Color(255, 255, 255, 127));
            beat_line.setPosition({timeline_x, static_cast<float>(next_beat_line_y)});
            view.draw(beat_line);
        }
        next_beat += 1;
        next_beat_line_y = beats_to_pixels_absolute.backwards_transform(next_beat);
    }

    float note_width = timeline_width / 16.f;
    float collizion_zone_width = note_width - 2.f;
    float long_note_rect_width = note_width * 0.75f;

    // Pre-size & center the shapes that can be
    tap_note_rect.setSize({note_width, 6.f});
    Toolbox::center(tap_note_rect);

    note_selected.setSize({note_width + 2.f, 8.f});
    Toolbox::center(note_selected);

    // Draw the notes
    auto draw_note = VariantVisitor {
        [&, this](const better::TapNote& tap_note){
            float note_x = timeline_x + note_width * (tap_note.get_position().index() + 0.5f);
            float note_y = static_cast<float>(beats_to_pixels_absolute.transform(tap_note.get_time()));
            const auto note_seconds = chart_state.chart.timing.time_at(tap_note.get_time());
            const auto first_colliding_beat = chart_state.chart.timing.beats_at(note_seconds - sf::milliseconds(500));
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto last_colliding_beat = chart_state.chart.timing.beats_at(note_seconds + sf::milliseconds(500));
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            note_collision_zone.setSize({collizion_zone_width, static_cast<float>(collision_zone_height)});
            Toolbox::set_local_origin_normalized(note_collision_zone, 0.5f, 0.f);
            note_collision_zone.setPosition(note_x, static_cast<float>(collision_zone_y));
            this->view.draw(note_collision_zone);
            tap_note_rect.setPosition(note_x, note_y);
            this->view.draw(tap_note_rect);
            if (chart_state.selected_notes.contains(tap_note)) {
                note_selected.setPosition(note_x, note_y);
                view.draw(note_selected);
            }
        },
        [&, this](const better::LongNote& long_note){
            float note_x = timeline_x + note_width * (long_note.get_position().index() + 0.5f);
            float note_y = static_cast<float>(beats_to_pixels_absolute.transform(long_note.get_time()));
            const auto note_start_seconds = chart_state.chart.timing.time_at(long_note.get_time());
            const auto first_colliding_beat = chart_state.chart.timing.beats_at(note_start_seconds - sf::milliseconds(500));
            const auto collision_zone_y = beats_to_pixels_absolute.transform(first_colliding_beat);
            const auto note_end_seconds = chart_state.chart.timing.time_at(long_note.get_end());
            const auto last_colliding_beat = chart_state.chart.timing.beats_at(note_end_seconds + sf::milliseconds(500));
            const auto collision_zone_height = beats_to_pixels_proportional.transform(last_colliding_beat - first_colliding_beat);
            note_collision_zone.setSize({collizion_zone_width, static_cast<float>(collision_zone_height)});
            Toolbox::set_local_origin_normalized(note_collision_zone, 0.5f, 0.f);
            note_collision_zone.setPosition(note_x, static_cast<float>(collision_zone_y));
            this->view.draw(note_collision_zone);
            const auto long_note_rect_height = beats_to_pixels_proportional.transform(long_note.get_duration());
            long_note_rect.setSize({long_note_rect_width, static_cast<float>(long_note_rect_height)});
            Toolbox::set_local_origin_normalized(long_note_rect, 0.5f, 0.f);
            long_note_rect.setPosition(note_x, note_y);
            this->view.draw(long_note_rect);
            tap_note_rect.setPosition(note_x, note_y);
            this->view.draw(tap_note_rect);
            if (chart_state.selected_notes.contains(long_note)) {
                note_selected.setPosition(note_x, note_y);
                this->view.draw(note_selected);
            }
        },
    };

    chart_state.chart.notes.in(
        first_visible_beat,
        last_visible_beat,
        [&](const better::Notes::iterator& it){
            it->second.visit(draw_note);
        }
    );

    if (chart_state.long_note_being_created.has_value()) {
        draw_note(make_long_note(*chart_state.long_note_being_created));
    }

    // Draw the cursor
    float cursor_width = timeline_width + 4.f;
    cursor.setSize({cursor_width, 4.f});
    view.draw(cursor);

    // Draw the time selection
    float selection_width = timeline_width;
    selection.setSize({selection_width, 0.f});
    if (chart_state.time_selection.has_value()) {
        const auto pixel_interval = Interval{
            beats_to_pixels_absolute.transform(chart_state.time_selection->start),
            beats_to_pixels_absolute.transform(chart_state.time_selection->end)
        };
        if (pixel_interval.intersects({0, y})) {
            selection.setSize({
                selection_width,
                static_cast<float>(pixel_interval.width()),
            });
            selection.setPosition(timeline_x, static_cast<float>(pixel_interval.start));
            view.draw(selection);
        }
    }
}

void LinearView::setZoom(int newZoom) {
    zoom = std::clamp(newZoom, -10, 10);
    reload_transforms();
}

void LinearView::displaySettings() {
    if (ImGui::Begin("Linear View Settings", &shouldDisplaySettings)) {
        Toolbox::editFillColor("Cursor", cursor);
        Toolbox::editFillColor("Note", tap_note_rect);
        Toolbox::editFillColor("Note Collision Zone", note_collision_zone);
        Toolbox::editFillColor("Long Note Tail", long_note_rect);
    }
    ImGui::End();
}

void LinearView::reload_transforms() {
    beats_to_pixels_proportional = {
        Fraction{0},
        Fraction{1, timeFactor()},
        Fraction{0},
        Fraction{100}
    };
}
