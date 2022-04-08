#pragma once

#include <filesystem>
#include <optional>

#include <tinyfiledialogs.h>

namespace feis {
    std::optional<std::filesystem::path> save_file_dialog();
    std::optional<std::filesystem::path> open_file_dialog();

    std::optional<std::filesystem::path> convert_char_array(const char* utf8_path);
}