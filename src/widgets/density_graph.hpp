#pragma once

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <string>
#include <filesystem>

#include "../better_song.hpp"
#include "../config.hpp"

class DensityGraph {
public:
    struct density_entry {
        int density;
        bool has_collisions;
    };

    DensityGraph(std::filesystem::path assets, const config::Config& config);
    sf::Texture base_texture;
    sf::Sprite normal_square;
    sf::Sprite collision_square;
    sf::RenderTexture graph;
    sf::FloatRect graph_rect;

    bool should_recompute = true;

    std::vector<DensityGraph::density_entry> densities;

    std::optional<int> last_height;
    std::optional<sf::Time> last_section_duration;

    void update(
        unsigned int height,
        const better::Chart& chart,
        const better::Timing& timing,
        const sf::Time& from,
        const sf::Time& to
    );

private:
    const std::filesystem::path texture_path;
    const sf::Time& collision_zone;

    void compute_densities(
        unsigned int height,
        const better::Chart& chart,
        const better::Timing& timing,
        const sf::Time& from,
        const sf::Time& to
    );
    void update_graph_texture();
};
