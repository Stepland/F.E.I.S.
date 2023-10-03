#include <folders.hpp>

#include <filesystem>
#include <functional>
#include <vector>

#include <sago/platform_folders.h>
#include <stdexcept>
#include <whereami++.hpp>

#include "compile_time_info.hpp"
#include "utf8_strings.hpp"

const std::vector<std::function<std::filesystem::path(void)>> assets_folder_candidates = {
    [](){ return utf8_encoded_string_to_path(whereami::executable_dir()) / "assets"; },
#ifdef __linux__
    [](){ return std::filesystem::path{"/usr/share"} / FEIS_DATA_FOLDER_NAME; },
#endif
};

std::filesystem::path choose_assets_folder() {
    for (const auto& folder_callback : assets_folder_candidates) {
        const auto& folder_candidate = folder_callback();
        if (std::filesystem::is_directory(folder_candidate)) {
            return folder_candidate;
        }
    }
    throw std::runtime_error("No suitable assets folder found");
}

const std::vector<std::function<std::filesystem::path(void)>> settings_folder_candidates = {
    [](){ return utf8_encoded_string_to_path(whereami::executable_dir()) / "settings"; },
#ifdef __linux__
    [](){ return utf8_encoded_string_to_path(sago::getConfigHome()) / FEIS_DATA_FOLDER_NAME; },
#endif
};

std::filesystem::path default_settings_folder() {
#ifdef __linux__
    return utf8_encoded_string_to_path(sago::getConfigHome()) / FEIS_DATA_FOLDER_NAME;
#else
    return utf8_encoded_string_to_path(whereami::executable_dir()) / "settings";
#endif
}

std::filesystem::path choose_settings_folder() {
    for (const auto& folder_callback : settings_folder_candidates) {
        const auto& folder_candidate = folder_callback();
        if (std::filesystem::is_directory(folder_candidate)) {
            return folder_candidate;
        }
    }
    return default_settings_folder();
}