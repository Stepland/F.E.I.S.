#include <filesystem>
#include <string>

std::string to_utf8_encoded_string(const std::u8string& u8s);
std::string to_utf8_encoded_string(const std::filesystem::path& path);
std::u8string to_u8string(const std::string& utf8s);
std::filesystem::path to_path(const std::string& utf8s);
std::string to_sfml_string(const std::filesystem::path& path);