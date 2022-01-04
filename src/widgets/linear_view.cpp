#include "linear_view.hpp"

#include <iostream>
#include <variant>

const std::string font_file = "fonts/NotoSans-Medium.ttf";

LinearView::LinearView(std::filesystem::path assets) :
    SecondsToTicks(-(60.f / last_BPM) / timeFactor(), 0.f, -last_resolution / timeFactor(), 0),
    SecondsToTicksProportional(0.f, (60.f / last_BPM), 0.f, last_resolution),
    PixelsToSeconds(-25.f, 75.f, -(60.f / last_BPM) / timeFactor(), 0.f),
    PixelsToSecondsProprotional(0.f, 100.f, 0.f, (60.f / last_BPM) / timeFactor()),
    PixelsToTicks(-25.f, 75.f, -last_resolution / timeFactor(), 0),
    font_path(assets / font_file)
{
    if (!beat_number_font.loadFromFile(font_path)) {
        std::cerr << "Unable to load " << font_path;
        throw std::runtime_error("Unable to load " + font_path.string());
    }

    cursor.setFillColor(sf::Color(66, 150, 250, 200));
    cursor.setOrigin(0.f, 2.f);
    cursor.setPosition({48.f, 75.f});

    selection.setFillColor(sf::Color(153, 255, 153, 92));
    selection.setOutlineColor(sf::Color(153, 255, 153, 189));
    selection.setOutlineThickness(1.f);

    note_rect.setFillColor(sf::Color(255, 213, 0, 255));

    note_selected.setFillColor(sf::Color(255, 255, 255, 200));
    note_selected.setOutlineThickness(1.f);

    note_collision_zone.setFillColor(sf::Color(230, 179, 0, 127));

    long_note_rect.setFillColor(sf::Color(255, 90, 0, 223));
    long_note_collision_zone.setFillColor(sf::Color(230, 179, 0, 127));
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
    const std::optional<Chart_with_History>& chart,
    const sf::Time& playbackPosition,
    const float& ticksAtPlaybackPosition,
    const float& BPM,
    const int& resolution,
    const ImVec2& size
) {
    int x = std::max(140, static_cast<int>(size.x));
    int y = std::max(140, static_cast<int>(size.y));

    resize(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
    reloadTransforms(playbackPosition, ticksAtPlaybackPosition, BPM, resolution);

    if (chart) {
        /*
         * Draw the beat lines and numbers
         */
        int next_beat_tick =
            ((1 + (static_cast<int>(PixelsToTicks.transform(0.f)) + resolution) / resolution) * resolution)
            - resolution;
        int next_beat = std::max(0, next_beat_tick / resolution);
        next_beat_tick = next_beat * resolution;
        float next_beat_line_y =
            PixelsToTicks.backwards_transform(static_cast<float>(next_beat_tick));

        sf::RectangleShape beat_line(sf::Vector2f(static_cast<float>(x) - 80.f, 1.f));

        sf::Text beat_number;
        beat_number.setFont(beat_number_font);
        beat_number.setCharacterSize(15);
        beat_number.setFillColor(sf::Color::White);
        std::stringstream ss;

        while (next_beat_line_y < static_cast<float>(y)) {
            if (next_beat % 4 == 0) {
                beat_line.setFillColor(sf::Color::White);
                beat_line.setPosition({50.f, next_beat_line_y});
                view.draw(beat_line);

                ss.str(std::string());
                ss << next_beat / 4;
                beat_number.setString(ss.str());
                sf::FloatRect textRect = beat_number.getLocalBounds();
                beat_number.setOrigin(
                    textRect.left + textRect.width,
                    textRect.top + textRect.height / 2.f);
                beat_number.setPosition({40.f, next_beat_line_y});
                view.draw(beat_number);

            } else {
                beat_line.setFillColor(sf::Color(255, 255, 255, 127));
                beat_line.setPosition({50.f, next_beat_line_y});
                view.draw(beat_line);
            }

            next_beat_tick += resolution;
            next_beat += 1;
            next_beat_line_y =
                PixelsToTicks.backwards_transform(static_cast<float>(next_beat_tick));
        }

        /*
         * Draw the notes
         */

        // Size & center the shapes
        float note_width = (static_cast<float>(x) - 80.f) / 16.f;
        note_rect.setSize({note_width, 6.f});
        Toolbox::center(note_rect);

        note_selected.setSize({note_width + 2.f, 8.f});
        Toolbox::center(note_selected);

        float collision_zone_size = PixelsToSecondsProprotional.backwards_transform(1.f);
        note_collision_zone.setSize(
            {(static_cast<float>(x) - 80.f) / 16.f - 2.f, collision_zone_size});
        Toolbox::center(note_collision_zone);

        long_note_collision_zone.setSize(
            {(static_cast<float>(x) - 80.f) / 16.f - 2.f, collision_zone_size});
        Toolbox::center(long_note_collision_zone);

        // Find the notes that need to be displayed
        int lower_bound_ticks = std::max(
            0,
            static_cast<int>(SecondsToTicks.transform(PixelsToSeconds.transform(0.f) - 0.5f)));
        int upper_bound_ticks = std::max(
            0,
            static_cast<int>(SecondsToTicks.transform(
                PixelsToSeconds.transform(static_cast<float>(y)) + 0.5f)));

        auto notes = chart->ref.getVisibleNotesBetween(lower_bound_ticks, upper_bound_ticks);
        auto currentLongNote = chart->makeCurrentLongNote();
        if (currentLongNote) {
            notes.insert(*currentLongNote);
        }

        for (auto& note : notes) {
            float note_x = 50.f + note_width * (note.getPos() + 0.5f);
            float note_y =
                PixelsToTicks.backwards_transform(static_cast<float>(note.getTiming()));
            note_rect.setPosition(note_x, note_y);
            note_selected.setPosition(note_x, note_y);
            note_collision_zone.setPosition(note_x, note_y);

            if (note.getLength() != 0) {
                float tail_size = PixelsToSecondsProprotional.backwards_transform(
                    SecondsToTicksProportional.backwards_transform(note.getLength()));
                float long_note_collision_size = collision_zone_size + tail_size;
                long_note_collision_zone.setSize(
                    {(static_cast<float>(x) - 80.f) / 16.f - 2.f, collision_zone_size});
                Toolbox::center(long_note_collision_zone);
                long_note_collision_zone.setSize(
                    {(static_cast<float>(x) - 80.f) / 16.f - 2.f, long_note_collision_size});
                long_note_collision_zone.setPosition(note_x, note_y);

                view.draw(long_note_collision_zone);

                float tail_width = .75f * (static_cast<float>(x) - 80.f) / 16.f;
                long_note_rect.setSize({tail_width, tail_size});
                sf::FloatRect long_note_bounds = long_note_rect.getLocalBounds();
                long_note_rect.setOrigin(
                    long_note_bounds.left + long_note_bounds.width / 2.f,
                    long_note_bounds.top);
                long_note_rect.setPosition(note_x, note_y);

                view.draw(long_note_rect);

            } else {
                view.draw(note_collision_zone);
            }

            view.draw(note_rect);

            if (chart->selectedNotes.find(note) != chart->selectedNotes.end()) {
                view.draw(note_selected);
            }
        }

        /*
         * Draw the cursor
         */
        cursor.setSize({static_cast<float>(x) - 76.f, 4.f});
        view.draw(cursor);

        /*
         * Draw the timeSelection
         */
        selection.setSize({static_cast<float>(x) - 80.f, 0.f});
        if (std::holds_alternative<unsigned int>(chart->timeSelection)) {
            unsigned int ticks = std::get<unsigned int>(chart->timeSelection);
            float selection_y =
                PixelsToTicks.backwards_transform(static_cast<float>(ticks));
            if (selection_y > 0.f and selection_y < static_cast<float>(y)) {
                selection.setPosition(50.f, selection_y);
                view.draw(selection);
            }
        } else if (std::holds_alternative<TimeSelection>(chart->timeSelection)) {
            const auto& ts = std::get<TimeSelection>(chart->timeSelection);
            float selection_start_y =
                PixelsToTicks.backwards_transform(static_cast<float>(ts.start));
            float selection_end_y = PixelsToTicks.backwards_transform(
                static_cast<float>(ts.start + ts.duration));
            if ((selection_start_y > 0.f and selection_start_y < static_cast<float>(y))
                or (selection_end_y > 0.f and selection_end_y < static_cast<float>(y))) {
                selection.setSize({static_cast<float>(x) - 80.f, selection_end_y - selection_start_y});
                selection.setPosition(50.f, selection_start_y);
                view.draw(selection);
            }
        }
    }
}

void LinearView::setZoom(int newZoom) {
    zoom = std::clamp(newZoom, -5, 5);
    shouldReloadTransforms = true;
}

void LinearView::displaySettings() {
    if (ImGui::Begin("Linear View Settings", &shouldDisplaySettings)) {
        Toolbox::editFillColor("Cursor", cursor);
        Toolbox::editFillColor("Note", note_rect);
        if (Toolbox::editFillColor("Note Collision Zone", note_collision_zone)) {
            long_note_collision_zone.setFillColor(note_collision_zone.getFillColor());
        }
        Toolbox::editFillColor("Long Note Tail", long_note_rect);
    }
    ImGui::End();
}

void LinearView::reloadTransforms(
    const sf::Time& playbackPosition,
    const float& ticksAtPlaybackPosition,
    const float& BPM,
    const int& resolution
) {
    if (shouldReloadTransforms or last_BPM != BPM or last_resolution != resolution) {
        shouldReloadTransforms = false;
        last_BPM = BPM;
        last_resolution = resolution;
        SecondsToTicksProportional =
            AffineTransform<float>(0.f, (60.f / BPM), 0.f, resolution);
        PixelsToSecondsProprotional =
            AffineTransform<float>(0.f, 100.f, 0.f, (60.f / BPM) / timeFactor());
        SecondsToTicks = AffineTransform<float>(
            playbackPosition.asSeconds() - (60.f / BPM) / timeFactor(),
            playbackPosition.asSeconds(),
            ticksAtPlaybackPosition - resolution / timeFactor(),
            ticksAtPlaybackPosition);
        PixelsToSeconds = AffineTransform<float>(
            -25.f,
            75.f,
            playbackPosition.asSeconds() - (60.f / BPM) / timeFactor(),
            playbackPosition.asSeconds());
        PixelsToTicks = AffineTransform<float>(
            -25.f,
            75.f,
            ticksAtPlaybackPosition - resolution / timeFactor(),
            ticksAtPlaybackPosition);
    } else {
        PixelsToSeconds.setB(
            playbackPosition.asSeconds() - 0.75f * (60.f / BPM) / timeFactor());
        PixelsToTicks.setB(ticksAtPlaybackPosition - 0.75f * resolution / timeFactor());
    }
}
