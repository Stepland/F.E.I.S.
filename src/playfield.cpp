#include "playfield.hpp"
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <stdexcept>
#include <variant>

#include "better_note.hpp"
#include "config.hpp"
#include "toolbox.hpp"
#include "utf8_sfml_redefinitions.hpp"
#include "utf8_strings.hpp"

const std::string texture_file = "textures/edit_textures/game_front_edit_tex_1.tex.png";

Playfield::Playfield(std::filesystem::path assets_folder) :
    long_note(assets_folder / "textures" / "long")
{
    if (sf::Shader::isAvailable()) {
        chord_tint_shader.emplace();
        const auto shader_path = assets_folder / "shaders" / "chord_tint.frag";
        if (not std::filesystem::is_regular_file(shader_path)) {
            throw std::runtime_error(fmt::format("File {} does not exist", path_to_utf8_encoded_string(shader_path)));
        }
        if (not chord_tint_shader->load_from_path(shader_path, sf::Shader::Fragment)) {
            throw std::runtime_error(fmt::format("Could not open fragment shader {}", path_to_utf8_encoded_string(shader_path)));
        };
        chord_tint_shader->setUniform("texture", sf::Shader::CurrentTexture);
    }
    const std::filesystem::path folder = assets_folder / "textures" / "playfield";
    const auto load_texture_and_sprite = [&](const std::string& file, feis::Texture& tex, sf::Sprite& sprite){
        const auto path = folder / file;
        if (not tex.load_from_path(path)) {
            throw std::runtime_error(fmt::format(
                "Unable to load texture {}",
                path_to_utf8_encoded_string(path)
            ));
        }
        tex.setSmooth(true);
        sprite.setTexture(tex);
    };
    load_texture_and_sprite("button.png", button_texture, button);
    load_texture_and_sprite("selected.png", note_selected_texture, note_selected);
    load_texture_and_sprite("collision.png", note_collision_texture, note_collision);

    const auto create_layer = [](sf::RenderTexture& tex, const std::string& name){
        if (not tex.create(400, 400)) {
            throw std::runtime_error("Unable to create " + name);
        }
        tex.setSmooth(true);
    };
    create_layer(long_note.layer, "long note layer");
    create_layer(chord_marker_layer, "chord marker layer");
    create_layer(note_numbers_layer, "note numbers layer");

    const auto font_path = assets_folder / "fonts" / "NotoSans-Bold.ttf";
    if (not note_numbers_font.load_from_path(font_path)) {
        throw std::runtime_error(fmt::format("Couldn't load font file {}", path_to_utf8_encoded_string(font_path)));
    }

}

void Playfield::resize(unsigned int width) {
    const auto _resize = [](auto& tex, unsigned int width){
        if (tex.getSize() != sf::Vector2u(width, width)) {
            if (not tex.create(width, width)) {
                throw std::runtime_error("Unable to resize Playfield texture");
            }
            tex.setSmooth(true);
        }
        tex.clear(sf::Color::Transparent);
    };

    _resize(long_note.layer, width);
    _resize(long_note_marker_layer, width);
    _resize(chord_marker_layer, width);
    _resize(note_numbers_layer, width);
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
                long_note.background.setTexture(*tex, true);
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

            {
                auto rect = long_note.tail.getTextureRect();
                rect.height = static_cast<int>(rect.height * tail_length_factor);
                long_note.tail.setTextureRect(rect);
                long_note.tail.setOrigin(rect.width / 2.f, -rect.width / 2.f);
                long_note.tail.setRotation(note.get_tail_angle() + 180);
                const float scale = square_size / rect.width;
                long_note.tail.setScale(scale, scale);
            }
            {
                auto rect = long_note.triangle.getTextureRect();
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
                const float scale = square_size / rect.width;
                long_note.triangle.setScale(scale, scale);
            }
            {
                auto rect = long_note.background.getTextureRect();
                long_note.background.setOrigin(rect.width / 2.f, rect.height / 2.f);
                long_note.background.setRotation(note.get_tail_angle());
                const float scale = square_size / rect.width;
                long_note.background.setScale(scale, scale);
            }
            {
                auto rect = long_note.outline.getTextureRect();
                long_note.outline.setOrigin(rect.width / 2.f, rect.height / 2.f);
                long_note.outline.setRotation(note.get_tail_angle());
                const float scale = square_size / rect.width;
                long_note.outline.setScale(scale, scale);
            }
            {
                auto rect = long_note.highlight.getTextureRect();
                long_note.highlight.setOrigin(rect.width / 2.f, rect.height / 2.f);
                const float scale = square_size / rect.width;
                long_note.highlight.setScale(scale, scale);
            }

            long_note.tail.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.triangle.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.background.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.outline.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);
            long_note.highlight.setPosition((x + 0.5f) * square_size, (y + 0.5f) * square_size);

            long_note.layer.draw(long_note.tail);
            long_note.layer.draw(long_note.background);
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
    const Marker& marker,
    const Judgement& marker_ending_state,
    const std::optional<config::Playfield>& chord_config
) {
    draw_tail_and_receptor(note, playback_position, timing);

    const float square_size = static_cast<float>(long_note.layer.getSize().x) / 4;
    const auto note_time = timing.time_at(note.get_time());
    const auto tail_end = timing.time_at(note.get_end());
    const auto offset = [&](){
        if (playback_position < tail_end) {
            return playback_position - note_time;
        } else {
            return playback_position - tail_end;
        }
    }();
    if (chord_config.has_value()) {
        draw_chord_tap_note(
            offset,
            note.get_position(),
            marker,
            marker_ending_state,
            *chord_config
        );
    } else {
        auto t = marker.at(marker_ending_state, offset);
        if (t) {
            const float x_scale = square_size / t->getTextureRect().width;
            const float y_scale = square_size / t->getTextureRect().height;
            t->setScale(x_scale, y_scale);
            t->setPosition(
                note.get_position().get_x() * square_size,
                note.get_position().get_y() * square_size
            );
            long_note_marker_layer.draw(*t);
        }
    }
}

void Playfield::draw_chord_tap_note(
    const better::TapNote& note,
    const sf::Time& playback_position,
    const better::Timing& timing,
    const Marker& marker,
    const Judgement& marker_ending_state,
    const config::Playfield& chord_config
) {
    const auto note_time = timing.time_at(note.get_time());
    const auto note_offset = playback_position - note_time;
    draw_chord_tap_note(
        note_offset,
        note.get_position(),
        marker,
        marker_ending_state,
        chord_config
    );
}

void Playfield::draw_note_number(
    const better::Note& note,
    const sf::Time& playback_position,
    const better::Timing& timing,
    const std::map<Fraction, unsigned int>& note_numbers,
    const config::Playfield& config
) {
    const auto note_time = timing.time_at(note.get_time());
    const auto note_offset = playback_position - note_time;
    if (
        note_offset >= sf::milliseconds(config.note_number_visibility_time_span.start)
        and note_offset < sf::milliseconds(config.note_number_visibility_time_span.end)
    ) {
        note_number.setFont(note_numbers_font);
        note_number.setString(fmt::format("{}", note_numbers.at(note.get_time())));
        const float square_size = chord_marker_layer.getSize().x / 4.0f;
        note_number.setCharacterSize(square_size * config.note_number_size);
        note_number.setFillColor(config.note_number_color);
        note_number.setOutlineThickness(note_number.getCharacterSize() * config.note_number_stroke_width / 5.f);
        note_number.setOutlineColor(config.note_number_stroke_color);
        Toolbox::center(note_number);
        const auto pos = note.get_position();
        note_number.setPosition(sf::Vector2f{pos.get_x() + 0.5f, pos.get_y() + 0.5f} * square_size);
        note_numbers_layer.draw(note_number);
    }
}

void Playfield::draw_chord_tap_note(
    const sf::Time& offset,
    const better::Position& position,
    const Marker& marker,
    const Judgement& marker_ending_state,
    const config::Playfield& chord_config
) {
    const float square_size = static_cast<float>(chord_marker_layer.getSize().x) / 4;
    auto t = marker.at(marker_ending_state, offset);
    if (t) {
        const float x_scale = square_size / t->getTextureRect().width;
        const float y_scale = square_size / t->getTextureRect().height;
        t->setScale(x_scale, y_scale);
        t->setPosition(
            position.get_x() * square_size,
            position.get_y() * square_size
        );
        if (chord_tint_shader) {
            chord_tint_shader->setUniform("tint", sf::Glsl::Vec4(chord_config.chord_color));
            chord_tint_shader->setUniform("mix_amount", chord_config.chord_color_mix_amount);
            chord_marker_layer.draw(*t, &chord_tint_shader.value());
        } else {
            t->setColor(chord_config.chord_color);
            chord_marker_layer.draw(*t);
        }
    }
}