#include "metadata_in_gui.hpp"

#include <filesystem>
#include <variant>
#include "src/better_song.hpp"

MetadataInGui::MetadataInGui(const better::Metadata& metadata) :
    title(metadata.title.value_or("")),
    artist(metadata.artist.value_or("")),
    audio(metadata.audio.value_or(std::filesystem::path{}).string()),
    jacket(metadata.jacket.value_or(std::filesystem::path{}).string())
{
    if (metadata.preview.has_value()) {
        if (std::holds_alternative<better::PreviewLoop>(*metadata.preview)) {
            const auto& p = std::get<better::PreviewLoop>(*metadata.preview);
            preview_loop = PreviewLoopInGui{p.get_start(), p.get_duration()};
            use_preview_file = false;
        } else if (std::holds_alternative<std::filesystem::path>(*metadata.preview)) {
            preview_file = std::get<std::filesystem::path>(*metadata.preview).string();
            use_preview_file = true;
        }
    }
}

MetadataInGui::operator better::Metadata() const {
    auto m = better::Metadata{};
    if (not title.empty()) {
        m.title = title;
    }
    if (not artist.empty()) {
        m.artist = artist;
    }
    if (not audio.empty()) {
        m.audio = audio;
    }
    if (not jacket.empty()) {
        m.jacket = jacket;
    }
    if (use_preview_file) {
        if (not preview_file.empty()) {
            m.preview = std::filesystem::path(preview_file);
        }
    } else if (preview_loop.start != preview_loop.duration) {
        m.preview = better::PreviewLoop{
            preview_loop.start,
            preview_loop.duration
        };
    }
    return m;
}