#include "playfield.hpp"

#include "../toolbox.hpp"

const std::string texture_file = "textures/edit_textures/game_front_edit_tex_1.tex.png";

Playfield::Playfield(std::filesystem::path assets_folder) :
    long_note(assets_folder / "textures" / "long"),
    texture_path(assets_folder / texture_file)
{
    if (!base_texture.loadFromFile(texture_path)) {
        std::cerr << "Unable to load texture " << texture_path;
        throw std::runtime_error("Unable to load texture " + texture_path.string());
    }
    base_texture.setSmooth(true);

    button.setTexture(base_texture);
    button.setTextureRect({0, 0, 192, 192});

    button_pressed.setTexture(base_texture);
    button_pressed.setTextureRect({192, 0, 192, 192});

    note_selected.setTexture(base_texture);
    note_selected.setTextureRect({384, 0, 192, 192});

    note_collision.setTexture(base_texture);
    note_collision.setTextureRect({576, 0, 192, 192});

    if (!marker_layer.create(400, 400)) {
        std::cerr << "Unable to create Playfield's markerLayer";
        throw std::runtime_error("Unable to create Playfield's markerLayer");
    }
    marker_layer.setSmooth(true);

    if (!long_note.layer.create(400, 400)) {
        std::cerr << "Unable to create Playfield's longNoteLayer";
        throw std::runtime_error("Unable to create Playfield's longNoteLayer");
    }
    long_note.layer.setSmooth(true);

    long_note.backgroud.setTexture(*long_note.marker.background_at(0));
    long_note.outline.setTexture(*long_note.marker.outline_at(0));
    long_note.highlight.setTexture(*long_note.marker.highlight_at(0));
    long_note.tail.setTexture(*long_note.marker.tail_at(0));
    long_note.triangle.setTexture(*long_note.marker.triangle_at(0));
}

void Playfield::resize(unsigned int width) {
    if (long_note.layer.getSize() != sf::Vector2u(width, width)) {
        if (!long_note.layer.create(width, width)) {
            std::cerr << "Unable to resize Playfield's longNoteLayer";
            throw std::runtime_error(
                "Unable to resize Playfield's longNoteLayer");
        }
        long_note.layer.setSmooth(true);
    }

    long_note.layer.clear(sf::Color::Transparent);

    if (marker_layer.getSize() != sf::Vector2u(width, width)) {
        if (!marker_layer.create(width, width)) {
            std::cerr << "Unable to resize Playfield's markerLayer";
            throw std::runtime_error(
                "Unable to resize Playfield's markerLayer");
        }
        marker_layer.setSmooth(true);
    }

    marker_layer.clear(sf::Color::Transparent);
}

void Playfield::draw_tail_and_receptor(
    const better::LongNote& note,
    const sf::Time& playbackPosition,
    const better::Timing& timing
) {
    float squareSize = static_cast<float>(long_note.layer.getSize().x) / 4;
    auto note_time = timing.time_at(note.get_time());
    auto note_offset = playbackPosition - note_time;
    auto frame = static_cast<int>(std::floor(note_offset.asSeconds() * 30.f));
    const auto x = note.get_position().get_x();
    const auto y = note.get_position().get_y();

    auto tail_end = timing.time_at(note.get_end());

    if (playbackPosition < tail_end) {
        // Before or During the long note
        if (auto tail_tex = long_note.marker.tail_at(frame)) {
            AffineTransform<float> OffsetToTriangleDistance(
                0.f,
                (tail_end - note_time).asSeconds(),
                static_cast<float>(note.get_tail_length()),
                0.f
            );

            
            long_note.tail.setTexture(*tail_tex, true);
            if (auto tex = long_note.marker.triangle_at(frame)) {
                long_note.triangle.setTexture(*tex, true);
            }
            if (auto tex = long_note.marker.background_at(frame)) {
                long_note.backgroud.setTexture(*tex, true);
            }
            if (auto tex = long_note.marker.outline_at(frame)) {
                long_note.outline.setTexture(*tex, true);
            }
            if (auto tex = long_note.marker.highlight_at(frame)) {
                long_note.highlight.setTexture(*tex, true);
            }

            auto rect = long_note.tail.getTextureRect();
            float tail_length_factor;

            if (frame < 8) {
                // Before the note : tail goes from triangle tip to note edge
                tail_length_factor = std::max(
                    0.f,
                    OffsetToTriangleDistance.clampedTransform(
                        note_offset.asSeconds()
                    ) - 1.f
                );
            } else {
                // During the note : tail goes from half of the triangle base to
                // note edge
                tail_length_factor = std::max(
                    0.f,
                    OffsetToTriangleDistance.clampedTransform(
                        note_offset.asSeconds()
                    ) - 0.5f
                );
            }

            rect.height = static_cast<int>(rect.height * tail_length_factor);
            long_note.tail.setTextureRect(rect);
            long_note.tail.setOrigin(rect.width / 2.f, -rect.width / 2.f);
            long_note.tail.setRotation(note.get_tail_angle() + 180);

            rect = long_note.triangle.getTextureRect();
            long_note.triangle.setOrigin(
                rect.width / 2.f,
                rect.width * (
                    0.5f
                    + OffsetToTriangleDistance.clampedTransform(
                        note_offset.asSeconds()
                    )
                )
            );
            long_note.triangle.setRotation(note.get_tail_angle());

            rect = long_note.backgroud.getTextureRect();
            long_note.backgroud.setOrigin(rect.width / 2.f, rect.height / 2.f);
            long_note.backgroud.setRotation(note.get_tail_angle());

            rect = long_note.outline.getTextureRect();
            long_note.outline.setOrigin(rect.width / 2.f, rect.height / 2.f);
            long_note.outline.setRotation(note.get_tail_angle());

            rect = long_note.highlight.getTextureRect();
            long_note.highlight.setOrigin(rect.width / 2.f, rect.height / 2.f);

            float scale = squareSize / rect.width;
            long_note.tail.setScale(scale, scale);
            long_note.triangle.setScale(scale, scale);
            long_note.backgroud.setScale(scale, scale);
            long_note.outline.setScale(scale, scale);
            long_note.highlight.setScale(scale, scale);

            long_note.tail.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
            long_note.triangle.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
            long_note.backgroud.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
            long_note.outline.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);
            long_note.highlight.setPosition((x + 0.5f) * squareSize, (y + 0.5f) * squareSize);

            long_note.layer.draw(long_note.tail);
            long_note.layer.draw(long_note.backgroud);
            long_note.layer.draw(long_note.outline);
            long_note.layer.draw(long_note.triangle);
            long_note.layer.draw(long_note.highlight);
        }
    }
}

void Playfield::draw_long_note(
    const better::LongNote& note,
    const sf::Time& playbackPosition,
    const better::Timing& timing,
    Marker& marker,
    MarkerEndingState& markerEndingState
) {
    draw_tail_and_receptor(note, playbackPosition, ticksAtPlaybackPosition, BPM, resolution);

    float squareSize = static_cast<float>(long_note.layer.getSize().x) / 4;

    AffineTransform<float> SecondsToTicksProportional(0.f, (60.f / BPM), 0.f, resolution);
    AffineTransform<float> SecondsToTicks(
        playbackPosition.asSeconds() - (60.f / BPM),
        playbackPosition.asSeconds(),
        ticksAtPlaybackPosition - resolution,
        ticksAtPlaybackPosition);

    float note_offset = SecondsToTicksProportional.backwards_transform(
        ticksAtPlaybackPosition - note.getTiming());
    int x = note.getPos() % 4;
    int y = note.getPos() / 4;

    float tail_end_in_seconds =
        SecondsToTicks.backwards_transform(note.getTiming() + note.getLength());
    float tail_end_offset = playbackPosition.asSeconds() - tail_end_in_seconds;

    if (playbackPosition.asSeconds() < tail_end_in_seconds) {
        // Before or During the long note
        // Display the beginning marker
        auto t = marker.getSprite(markerEndingState, note_offset);
        if (t) {
            float scale = squareSize / t->get().getSize().x;
            marker_sprite.setTexture(*t, true);
            marker_sprite.setScale(scale, scale);
            marker_sprite.setPosition(x * squareSize, y * squareSize);
            marker_layer.draw(markerSprite);
        }

    } else {
        // After long note end : Display the ending marker
        if (tail_end_offset >= 0.0f) {
            auto t = marker.getSprite(markerEndingState, tail_end_offset);
            if (t) {
                float scale = squareSize / t->get().getSize().x;
                marker_sprite.setTexture(*t, true);
                marker_sprite.setScale(scale, scale);
                marker_sprite.setPosition(x * squareSize, y * squareSize);
                marker_layer.draw(markerSprite);
            }
        }
    }
}