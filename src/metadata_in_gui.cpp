#include "metadata_in_gui.hpp"

#include <filesystem>
#include <variant>

MetadataInGui::MetadataInGui(const better::Metadata& metadata) :
    title(metadata.title.value_or("")),
    artist(metadata.artist.value_or("")),
    audio(metadata.audio.value_or(std::filesystem::path{}).string()),
    jacket(metadata.jacket.value_or(std::filesystem::path{}).string()),
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