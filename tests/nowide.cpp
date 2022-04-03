#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <string>
#include <tinyfiledialogs.h>
#include <nowide/args.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>

int main() {
    const char* _filepath = tinyfd_openFileDialog(
        "Open File", nullptr, 0, nullptr, nullptr, false
    );
    if (_filepath == nullptr) {
        nowide::cerr << "No file chosen" << std::endl;
    }
    nowide::cout << "_filepath received, seen through nowide::cout : " << _filepath << std::endl;
    std::cout << "_filepath received, seen through std::cout : " << _filepath << std::endl;

    auto u8string = std::u8string(_filepath, _filepath + std::strlen(_filepath));

    nowide::cout << "converted to std::u8string, seen through nowide::cout : " << reinterpret_cast<const char*>(u8string.c_str()) << std::endl;
    std::cout << "converted to std::u8string, seen through std::cout : " << reinterpret_cast<const char*>(u8string.c_str()) << std::endl;

    auto filepath = std::filesystem::path{_filepath};

    nowide::cout << "_filepath passed to std::filesystem::path, seen through nowide::cout : " << filepath << std::endl;
    std::cout << "_filepath passed to std::filesystem::path, seen through std::cout : " << filepath << std::endl;

    auto u8path = std::filesystem::path{u8string};

    nowide::cout << "u8string passed to std::filesystem::path, seen through nowide::cout : " << u8path << std::endl;
    std::cout << "u8string passed to std::filesystem::path, seen through std::cout : " << u8path << std::endl; 

    nowide::ifstream f(filepath.string().c_str()); // argv[1] - is UTF-8
    if(not f) {
        nowide::cerr << "Can't open " << filepath << std::endl;
        return 1;
    }
    std::size_t total_lines = 0;
    while (f) {
        if(f.get() == '\n') {
            total_lines++;
        }
    }
    nowide::cout << "File " << filepath << " has " << total_lines << " lines" << std::endl;
    return 0;
}