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


void print_each_char_fmt(const std::string& text) {
    fmt::print("full string : {}\n", text);
    fmt::print("one char at a time : \n");
    for (const auto& c : text) {
        const int char_value = static_cast<int>(c);
        fmt::print("{} (int) {:x} (hex)\n", c, char_value, char_value);
    }
}

void print_each_char_nowide(const std::string& text) {
    nowide::cout << "full string : " << text << std::endl;
    nowide::cout << "one char at a time :" << std::endl;
    for (const auto& c : text) {
        const int char_value = static_cast<int>(c);
        nowide::cout << std::dec << char_value << " (int) " << std::hex << char_value << " (hex)" << std::endl;
    }
}

int main() {
    std::string executable_folder = whereami::executable_dir();
    fmt::print("whereami::executable_dir() (throught fmt::print)\n");
    print_each_char_fmt(executable_folder);

    fmt::print("whereami::executable_dir() (throught nowide::cout)\n");
    print_each_char_nowide(executable_folder);
    return 0;
}