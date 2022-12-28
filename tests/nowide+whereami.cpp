#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <string>
#include <nowide/args.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>
#include <whereami++.hpp>

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
    std::string executable_folder = whereami::executable_dir();
    nowide::cout << "whereami::executable_dir() = " << executable_folder << std::endl;
	
    auto u8string = std::u8string(executable_folder.begin(), executable_folder.end());
    nowide::cout << "std::u8string{_)} = " << reinterpret_cast<const char*>(u8string.c_str()) << std::endl;

    std::filesystem::path u8path{u8string};
    nowide::cout << "std::filesystem::path{_} = " << u8path << std::endl;

	nowide::cout << "nowide::ifstream{_} : " << std::endl;
	nowide::ifstream file_from_path(u8path / "test_file.txt");
	check_file(file_from_path);
    return 0;
}