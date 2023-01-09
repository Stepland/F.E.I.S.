#include "utf8_sfml.hpp"
#include <filesystem>
#include <ios>
#include "nowide/fstream.hpp"

#include "utf8_strings.hpp"

feis::UTF8FileStream::UTF8FileStream(const std::filesystem::path& file) :
    nowide_stream(to_utf8_encoded_string(file)),
    path(file)
{}

sf::Int64 feis::UTF8FileStream::read(void* data, sf::Int64 size) {
    try {
        nowide_stream.read(static_cast<char*>(data), size);
    } catch (const std::ios_base::failure& e) {
        return -1;
    }
    return nowide_stream.gcount();
}

sf::Int64 feis::UTF8FileStream::seek(sf::Int64 position) {
    try {
        nowide_stream.seekg(position, std::ios_base::beg);
        return nowide_stream.tellg();
    } catch (const std::ios_base::failure& e) {
        return -1;
    }
}

sf::Int64 feis::UTF8FileStream::tell() {
    try {
        return nowide_stream.tellg();
    } catch (const std::ios_base::failure& e) {
        return -1;
    }
}

sf::Int64 feis::UTF8FileStream::getSize() {
    try {
        return std::filesystem::file_size(path);
    } catch(std::filesystem::filesystem_error const& ex) {
        return -1;
    }
}

bool feis::Music::open_from_file(const std::filesystem::path& file) {
    file_stream.emplace(to_utf8_encoded_string(file));
    return this->openFromStream(*file_stream);
}