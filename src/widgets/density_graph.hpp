#ifndef FEIS_DENSITYGRAPH_H
#define FEIS_DENSITYGRAPH_H

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <string>
#include <filesystem>

#include "../chart.hpp"

class DensityGraph {
public:
    struct density_entry {
        int density;
        bool has_collisions;
    };

    DensityGraph(std::filesystem::path assets);
    sf::Texture base_texture;
    sf::Sprite normal_square;
    sf::Sprite collision_square;
    sf::RenderTexture graph;
    sf::FloatRect graph_rect;

    bool should_recompute = true;

    std::vector<DensityGraph::density_entry> densities;

    std::optional<int> last_height;
    std::optional<float> last_section_length;

    void computeDensities(int height, float chartRuntime, Chart& chart, float BPM, int resolution);
    void updateGraphTexture();

private:
    const std::filesystem::path texture_path;
};

#endif  // FEIS_DENSITYGRAPH_H
