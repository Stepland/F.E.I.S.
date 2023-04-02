#include "playfield.hpp"

#include "toolbox.hpp"
#include "utf8_strings.hpp"

const std::string texture_file = "textures/edit_textures/game_front_edit_tex_1.tex.png";

Playfield::Playfield(std::filesystem::path assets_folder) :
    long_note(assets_folder / "textures" / "long"),
    texture_path(assets_folder / texture_file)
{
    if (!base_texture.load_from_path(texture_path)) {
        std::cerr << "Unable to load texture " << texture_path;
        throw std::runtime_error("Unable to load texture " + path_to_utf8_encoded_string(texture_path));
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

    // why do we do this here ?
    long_note.backgroud.setTexture(*long_note.marker.background_at(sf::Time::Zero));
    long_note.outline.setTexture(*long_note.marker.outline_at(sf::Time::Zero));
    long_note.highlight.setTexture(*long_note.marker.highlight_at(sf::Time::Zero));
    long_note.tail.setTexture(*long_note.marker.tail_at(sf::Time::Zero));
    long_note.triangle.setTexture(*long_note.marker.triangle_at(sf::Time::Zero));
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
    const sf::Time& playback_position,
    const better::Timing& timing
) {
    const float square_size = static_cast<float>(long_note.layer.getSize().x) / 4;
    const auto note_time = timing.time_at(note.get_time());
    const auto note_offset = playback_position - note_time;
    const auto x = note.get_position().get_x();
    const auto y = note.get_position().get_y();

    const auto tail_end = timing.time_at(note.get_end());

    if (playback_position < tail_end) {
        // Before or During the long note
        if (auto tail_tex = long_note.marker.tail_at(note_offset)) {
            AffineTransform<float> OffsetToTriangleDistance(
                0.f,
                (tail_end - note_time).asSeconds(),
                static_cast<float>(note.get_tail_length()),
                0.f
            );
            
            long_note.tail.setTexture(*tail_tex, true);
            if (auto tex = long_note.marker.triangle_at(note_offset)) {
                long_note.triangle.setTexture(*tex, true);
            }
            if (auto tex = long_note.marker.background_at(note_offset)) {
                long_note.backgroud.setTexture(*tex, true);
            }
            if (auto tex = long_note.marker.outline_at(note_offset)) {
                long_note.outline.setTexture(*tex, true);
            }
            if (auto tex = long_note.marker.highlight_at(note_offset)) {
                long_note.highlight.setTexture(*tex, true);
            }
            

            /*
            The triangle textures used before the triangle animation cycle
            kicks in already have the tail baked in so the protion of the tail
            we draw ourself can stop at the exact border of the triangle
            texture without leaving a visible edge.

            On the other hand, the textures of the triangle animation cycle do
            NOT have the tail backed in, so the portion of the tail we draw
            ourselves has to extend further behind the triangle to hide its
            edge
            */
            float tail_length_factor;
            if (not long_note.marker.triangle_cycle_displayed_at(note_offset)) {
                // Before the cycle : tail goes from triangle tip to note edge
                tail_length_factor = std::max(
                    0.f,
                    OffsetToTriangleDistance.clampedTransform(
                        note_offset.asSeconds()
                    ) - 1.f
                );
            } else {
                // During the cycle : tail goes from half of the triangle base to
                // note edge
                tail_length_factor = std::max(
                    0.f,
                    OffsetToTriangleDistance.clampedTransform(
                        note_offset.asSeconds()
                    ) - 0.5f
                );
            }

            auto rect = long_note.tail.getTextureRect();
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

            const float scale = square_size / rect.width;
            long_note.tail.setScale(scale, scale);
            long_note.triangle.setScale(scale, scale);
            long_note.backgroud.setScale(scale, scale);
            long_note.outline.setScale(scale, scale);
            long_note.highlight.setScale(scale, scale);

            long_note.tail.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.triangle.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.backgroud.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.outline.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.highlight.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);

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
    const sf::Time& playback_position,
    const better::Timing& timing,
    Marker& marker,
    Judgement& markerEndingState
) {
    draw_tail_and_receptor(note, playback_position, timing);

    const float square_size = static_cast<float>(long_note.layer.getSize().x) / 4;
    const auto note_time = timing.time_at(note.get_time());
    const auto note_offset = playback_position - note_time;

    const auto tail_end = timing.time_at(note.get_end());
    if (playback_position < tail_end) {
        // Before or During the long note
        // Display the beginning marker
        auto t = marker.at(markerEndingState, note_offset);
        if (t) {
            const float scale = square_size / t->get().getSize().x;
            marker_sprite.setTexture(*t, true);
            marker_sprite.setScale(scale, scale);
            marker_sprite.setPosition(
                note.get_position().get_x() * square_size,
                note.get_position().get_y() * square_size
            );
            marker_layer.draw(marker_sprite);
        }

    } else {
        const auto tail_end_offset = playback_position - tail_end;
        auto t = marker.at(markerEndingState, tail_end_offset);
        if (t) {
            const float scale = square_size / t->get().getSize().x;
            marker_sprite.setTexture(*t, true);
            marker_sprite.setScale(scale, scale);
            marker_sprite.setPosition(
                note.get_position().get_x() * square_size,
                note.get_position().get_y() * square_size
            );
            marker_layer.draw(marker_sprite);
        }
    }
}