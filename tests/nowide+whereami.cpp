#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include <nowide/args.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>
#include <whereami++.hpp>
#include <fmt/core.h>

void print_each_char(const std::u8string& text) {
    nowide::cout << "full string : " << reinterpret_cast<const char*>(text.c_str()) << std::endl;
    nowide::cout << "one char at a time :" << std::endl;
    for (const auto& c : text) {
        const int char_value = static_cast<int>(c);
        nowide::cout << std::dec << char_value << " (int) " << std::hex << char_value << " (hex)" << std::endl;
    }
}

int main() {
    std::string exec_dir = whereami::executable_dir();
    std::u8string exec_dir_u8{exec_dir.begin(), exec_dir.end()};
    nowide::cout << "exec dir converted to u8string : " << std::endl;
    print_each_char(exec_dir_u8);
    return 0;
}