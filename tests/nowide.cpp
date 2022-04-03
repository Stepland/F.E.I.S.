#include <cstddef>
#include <filesystem>

#include <tinyfiledialogs.h>
#include <nowide/args.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>

int main(int argc, char** argv) {
    const char* _filepath = tinyfd_openFileDialog(
        "Open File", nullptr, 0, nullptr, nullptr, false
    );
    if (_filepath == nullptr) {
        nowide::cerr << "No file chosen" << std::endl;
    }
    auto filepath = std::filesystem::path{_filepath};
    nowide::ifstream f(filepath.string().c_str()); // argv[1] - is UTF-8
    if(not f) {
        nowide::cerr << "Can't open " << argv[1] << std::endl;
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