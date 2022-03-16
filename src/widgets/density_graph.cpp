#include "density_graph.hpp"

#include <algorithm>

const std::string texture_file = "textures/edit_textures/game_front_edit_tex_1.tex.png";

DensityGraph::DensityGraph(std::filesystem::path assets) :
    texture_path(assets / texture_file)
{
    if (!base_texture.loadFromFile(texture_path)) {
        std::cerr << "Unable to load texture " << texture_path;
        throw std::runtime_error("Unable to load texture " + texture_path.string());
    }
    base_texture.setSmooth(true);

    normal_square.setTexture(base_texture);
    normal_square.setTextureRect({456, 270, 6, 6});

    collision_square.setTexture(base_texture);
    collision_square.setTextureRect({496, 270, 6, 6});
}

void DensityGraph::update(int height, const better::Chart& chart, const sf::Time& from, const sf::Time& to) {
    this->compute_densities(height, chart, from, to);
    this->update_graph_texture();
}

void DensityGraph::compute_densities(int height, const better::Chart& chart, const sf::Time& from, const sf::Time& to) {
    last_height = height;

    // minus the slider cursor thiccccnesss
    int available_height = height - 10;
    int sections = (available_height + 1) / 5;
    densities.clear();

    if (sections >= 1) {
        densities.resize(static_cast<unsigned int>(sections), {0, false});

        sf::Time section_duration = (to - from) / static_cast<sf::Int64>(sections);
        last_section_duration = section_duration;

        for (auto const& note : chart.notes) {
            auto note_time = note.getTiming();
            auto note_seconds = chart.timing.time_at(ticksToSeconds(note_time));
            auto float_section = note_seconds / section_duration;
            auto int_section = static_cast<int>(float_section);
            auto section = std::clamp(int_section, 0, sections - 1);
            densities.at(section).density += 1;
            if (not densities.at(section).has_collisions) {
                densities.at(section).has_collisions =
                    chart.is_colliding(note, ticks_threshold);
            }
        }
    }
}

void DensityGraph::update_graph_texture() {
    if (!graph.create(45, static_cast<unsigned int>(*last_height))) {
        std::cerr << "Unable to create DensityGraph's RenderTexture";
        throw std::runtime_error(
            "Unable to create DensityGraph's RenderTexture");
    }
    graph_rect = {0.0, 0.0, 45.0, static_cast<float>(*last_height)};
    graph.clear(sf::Color::Transparent);
    graph.setSmooth(true);

    unsigned int x = 2;
    unsigned int y = 4;

    unsigned int line = 0;

    for (auto const& density_entry : densities) {
        if (density_entry.has_collisions) {
            for (int col = 0; col < density_entry.density; ++col) {
                collision_square.setPosition(x + col * 5, y + line * 5);
                graph.draw(collision_square);
            }
        } else {
            for (int col = 0; col < std::min(8, density_entry.density); ++col) {
                normal_square.setPosition(x + col * 5, y + line * 5);
                graph.draw(normal_square);
            }
        }
        ++line;
    }
}