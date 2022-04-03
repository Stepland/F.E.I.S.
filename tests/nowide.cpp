#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <string>
#include <tinyfiledialogs.h>
#include <nowide/args.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>

template <class T>
void check_file(T& file) {
	if (not file) {
        nowide::cerr << "Can't open file" << std::endl;
        return;
    }
    std::size_t total_lines = 0;
    while (file) {
        if(file.get() == '\n') {
            total_lines++;
        }
    }
    nowide::cout << "File has " << total_lines << " lines" << std::endl;
}

int main() {
    const char* _filepath = tinyfd_openFileDialog(
        "Open File", nullptr, 0, nullptr, nullptr, false
    );
    if (_filepath == nullptr) {
        nowide::cerr << "No file chosen" << std::endl;
		return 0;
    }
    nowide::cout << "const char* _filepath = " << _filepath << std::endl;
	
    auto u8string = std::u8string(_filepath, _filepath + std::strlen(_filepath));
    nowide::cout << "std::u8string{_filepath)} = " << reinterpret_cast<const char*>(u8string.c_str()) << std::endl;

    std::filesystem::path u8path{u8string};
    nowide::cout << "std::filesystem::path{u8string} = " << u8path << std::endl;

	nowide::cout << "nowide::ifstream{u8path} : " << std::endl;
	nowide::ifstream file_from_path(u8path.string());
	check_file(file_from_path);
    return 0;
}