#include "utf8_strings.hpp"
#include <SFML/System/String.hpp>

std::string to_utf8_encoded_string(const std::u8string& u8s) {
    std::string result{u8s.cbegin(), u8s.cend()};
    return result;
}

std::string to_utf8_encoded_string(const std::filesystem::path& path) {
    return to_utf8_encoded_string(path.u8string());
}

std::u8string to_u8string(const std::string& utf8s) {
    std::u8string result{utf8s.cbegin(), utf8s.cend()};
    return result;
}

std::filesystem::path to_path(const std::string& utf8s) {
    return std::filesystem::path{to_u8string(utf8s)};
}

std::string to_sfml_string(const std::filesystem::path& path) {
#ifdef _WIN32
    const auto string_path = to_utf8_encoded_string(path);
    const auto sfml_string_path = sf::String::fromUtf8(string_path.begin(), string_path.end());
    return sfml_string_path.toAnsiString();
#else
    return path.string();
#endif
}