#pragma once

#include <filesystem>
#include <future>
#include <map>
#include <optional>

#include "config.hpp"
#include "marker.hpp"
#include "utf8_sfml_redefinitions.hpp"

/* Deals with loading markers and marker previews asynchronously */
class Markers {
public:

    using marker_type = std::optional<std::shared_ptr<Marker>>;
    using marker_loader_type = std::future<marker_type>;
    using preview_type = std::optional<feis::Texture>;
    using previews_type = std::map<std::filesystem::path, preview_type>;

    Markers(
        const std::filesystem::path& markers_folder,
        config::Config& config
    );

    void load_marker(const std::filesystem::path& chosen);
    
    const marker_type& get_chosen_marker() const;
    
    previews_type::const_iterator cbegin() const;
    previews_type::const_iterator cend() const;
    
    void update();

    void display_debug();
private:
    std::filesystem::path markers_folder;
    config::Config& config;

    // Markers
    marker_type chosen_marker;
    std::tuple<std::filesystem::path, std::future<marker_type>> marker_loader;
    static marker_type load_default_marker(
        const std::optional<std::filesystem::path>& marker_path_from_config,
        const std::filesystem::path& markers_folder
    );
    static marker_type load_marker_async(
        const std::filesystem::path& chosen,
        const std::filesystem::path& markers
    );
    std::vector<std::future<void>> discarded_markers;
    static void discard_marker(std::future<marker_type>&& future);

    previews_type previews;
    std::map<std::filesystem::path, std::future<preview_type>> preview_loaders;

    static preview_type load_marker_preview(const std::filesystem::path& folder);
};