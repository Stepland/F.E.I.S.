#include "markers.hpp"

#include <future>
#include <ranges>

#include "fmt/core.h"
#include "imgui.h"

#include "marker.hpp"
#include "utf8_strings.hpp"

Markers::Markers(
    const std::filesystem::path& markers_folder_,
    config::Config& config_
) :
    markers_folder(markers_folder_),
    config(config_)
{

    auto contents = std::filesystem::directory_iterator(markers_folder);
    auto subdirs = std::views::filter(contents, [](const auto& dir_it){return dir_it.is_directory();});

    for (const auto& dir_it : subdirs) {
        const auto folder = std::filesystem::relative(dir_it, markers_folder);
        preview_loaders[folder] = std::async(std::launch::async, load_marker_preview, dir_it.path());
    }

    marker_loader = std::make_tuple(
        std::filesystem::path{},
        std::async(std::launch::async, load_default_marker, std::cref(config.marker.folder), std::cref(markers_folder))
    );
}

void Markers::load_marker(const std::filesystem::path& chosen) {
    auto& [path, future] = marker_loader;
    if (future.valid()) {
        discarded_markers.emplace_back(std::async(std::launch::async, discard_marker, std::move(future)));
    }
    marker_loader = std::make_tuple(
        chosen,
        std::async(std::launch::async, load_marker_async, std::cref(chosen), std::cref(markers_folder))
    );
}

const Markers::marker_type& Markers::get_chosen_marker() const {
    return chosen_marker;
}

Markers::previews_type::const_iterator Markers::cbegin() const {
    return previews.cbegin();
}

Markers::previews_type::const_iterator Markers::cend() const {
    return previews.cend();
}

void Markers::update() {
    std::erase_if(preview_loaders, [](const auto& it){
        return not it.second.valid();
    });
    for (auto& [path, future] : preview_loaders) {
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            previews[path] = future.get();
        }
    }
    {
        auto& [path, future] = marker_loader;
        if (future.valid()) {
            if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                const auto loaded_marker = future.get();
                if (loaded_marker) {
                    chosen_marker = loaded_marker;
                    config.marker.folder = path;
                } else {
                    previews.erase(path);
                }
            }
        }
    }

    std::erase_if(discarded_markers, [](const std::future<void>& f){
        return not f.valid() or f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    });
}

void Markers::display_debug() {
    if (ImGui::Begin("Markers Debug")) {
        ImGui::TextUnformatted("chosen_marker :");
        ImGui::SameLine();
        if (not chosen_marker) {
            ImGui::TextDisabled("(empty)");
        } else {
            ImGui::TextUnformatted(path_to_utf8_encoded_string((*chosen_marker)->get_folder()).c_str());
        }
        ImGui::TextUnformatted("marker_loader :");
        ImGui::SameLine();
        if (not std::get<1>(marker_loader).valid()) {
            ImGui::TextDisabled("(empty)");
        } else {
            ImGui::TextUnformatted(path_to_utf8_encoded_string(std::get<0>(marker_loader)).c_str());
        }
        if (ImGui::TreeNodeEx("discarded_markers", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& future : discarded_markers) {
                ImGui::BulletText("%s", fmt::format("{}", fmt::ptr(&future)).c_str());
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("previews", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& [path, _] : previews) {
                ImGui::BulletText("%s", fmt::format("{}", path_to_utf8_encoded_string(path)).c_str());
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("preview_loaders", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& [path, _] : preview_loaders) {
                ImGui::BulletText("%s", fmt::format("{}", path_to_utf8_encoded_string(path)).c_str());
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

Markers::marker_type Markers::load_default_marker(
    const std::optional<std::filesystem::path>& chosen_from_config,
    const std::filesystem::path& markers
) {
    if (chosen_from_config and chosen_from_config->is_relative()) {
        try {
            return load_marker_from(markers / *chosen_from_config);
        } catch (const std::exception& e) {
            fmt::print("Failed to load marker from preferences");
        }
    }
    return first_available_marker_in(markers);
}

Markers::marker_type Markers::load_marker_async(const std::filesystem::path& chosen, const std::filesystem::path& markers) {
    try {
        return load_marker_from(markers / chosen);
    } catch (const std::exception& e) {
        return {};
    }
}

void Markers::discard_marker(std::future<marker_type>&& future) {
    auto other_future = std::move(future);
    if (other_future.valid()) {
        other_future.get();
    }
}

Markers::preview_type Markers::load_marker_preview(const std::filesystem::path& chosen) {
    for (const auto& file : {"preview.png", "ma15.png"}) {
        const auto preview_path = chosen / file;
        if (std::filesystem::exists(preview_path)) {
            feis::Texture preview;
            if (preview.load_from_path(preview_path)) {
                return preview;
            }
        }
    }
    return {};
}