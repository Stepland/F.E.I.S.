#include "file_dialogs.hpp"
#include "utf8_strings.hpp"

#include <cstring>

namespace feis {
    std::optional<std::filesystem::path> save_file_dialog() {
        char const* options[1] = {"*.memon"};
        return convert_char_array(tinyfd_saveFileDialog(
            "Save File",
            nullptr,
            1,
            options,
            nullptr
        ));
    };

    std::optional<std::filesystem::path> open_file_dialog() {
        return convert_char_array(tinyfd_openFileDialog(
            "Open File", 
            nullptr, 
            0,
            nullptr, 
            nullptr, 
            false
        ));

    };

    std::optional<std::filesystem::path> convert_char_array(const char* utf8_path) {
        if (utf8_path == nullptr) {
            return {};
        } else {
            const std::string utf8_string{utf8_path, utf8_path+std::strlen(utf8_path)};
            return utf8_encoded_string_to_path(utf8_string);
        }
    }
}