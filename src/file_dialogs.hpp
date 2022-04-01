#pragma once

#include <filesystem>
#include <optional>

#include <tinyfiledialogs.h>

namespace feis {
    std::optional<std::filesystem::path> ask_for_save_path();
}