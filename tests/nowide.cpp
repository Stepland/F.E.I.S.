#include <cstddef>

#include <nowide/args.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>

int main(int argc, char** argv) {
    nowide::args _{argc, argv}; // Fix arguments - make them UTF-8
    if(argc != 2) {
        nowide::cerr << "usage: " << argv[0] << " file" << std::endl; // Unicode aware console
        return 1;
    }

    nowide::ifstream f(argv[1]); // argv[1] - is UTF-8
    if(not f) {
        // the console can display UTF-8
        nowide::cerr << "Can't open " << argv[1] << std::endl;
        return 1;
    }
    std::size_t total_lines = 0;
    while (f) {
        if(f.get() == '\n') {
            total_lines++;
        }
    }
    // the console can display UTF-8
    nowide::cout << "File " << argv[1] << " has " << total_lines << " lines" << std::endl;
    return 0;
}