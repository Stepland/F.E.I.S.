/* Copied from src/SFML/System/UTF8FileInputStream.cpp */

#include "utf8_file_input_stream.hpp"

#include <cstddef>
#include <memory>

#include <nowide/cstdio.hpp>

#include "utf8_strings.hpp"


void feis::UTF8FileInputStream::FileCloser::operator()(std::FILE* file) {
    std::fclose(file);
}

bool feis::UTF8FileInputStream::open(const std::filesystem::path& filename) {
    m_file.reset(nowide::fopen(path_to_utf8_encoded_string(filename).c_str(), "rb"));
    return m_file != nullptr;
}

sf::Int64 feis::UTF8FileInputStream::read(void* data, sf::Int64 size) {
    if (m_file) {
        return static_cast<sf::Int64>(std::fread(data, 1, static_cast<std::size_t>(size), m_file.get()));
    } else {
        return -1;
    }
}

sf::Int64 feis::UTF8FileInputStream::seek(sf::Int64 position) {
    if (m_file) {
        if (std::fseek(m_file.get(), static_cast<long>(position), SEEK_SET)) {
            return -1;
        }
        
        return tell();
    } else {
        return -1;
    }
}

sf::Int64 feis::UTF8FileInputStream::tell() {
    if (m_file) {
        return std::ftell(m_file.get());
    } else {
        return -1;
    }
}

sf::Int64 feis::UTF8FileInputStream::getSize() {
    if (m_file) {
        sf::Int64 position = tell();
        std::fseek(m_file.get(), 0, SEEK_END);
        sf::Int64 size = tell();
        if (seek(position) == -1) {
            return -1;
        }
        return size;
    } else {
        return -1;
    }
}
