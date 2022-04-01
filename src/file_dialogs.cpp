#include "file_dialogs.hpp"

namespace feis {
    std::optional<std::filesystem::path> ask_for_save_path() {
        char const* options[1] = {"*.memon"};
        const auto raw_path = tinyfd_saveFileDialog(
            "Save File",
            nullptr,
            1,
            options,
            nullptr
        );
        if (raw_path == nullptr) {
            return {};
        } else {
            return std::filesystem::path{raw_path};
        }
    }
}