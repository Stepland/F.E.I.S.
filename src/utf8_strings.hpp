#include <filesystem>
#include <string>

std::string u8string_to_utf8_encoded_string(const std::u8string& u8s);
std::string path_to_utf8_encoded_string(const std::filesystem::path& path);
std::u8string to_u8string(const std::string& utf8s);
std::filesystem::path to_path(const std::string& utf8s);