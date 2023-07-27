#include "error_messages.hpp"

#include <cstdlib>

#include <tinyfiledialogs.h>

#include "utf8_strings.hpp"

void crash_if_missing(const std::filesystem::path& file) {
    if (not std::filesystem::exists(file)) {
        tinyfd_messageBox(
            "Error",
            ("Could not find " + path_to_utf8_encoded_string(file)).c_str(),
            "ok",
            "error",
            1);
        std::exit(EXIT_FAILURE);
    }
}