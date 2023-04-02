#include "toolbox.hpp"

#include <cmath>
#include <fstream>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iomanip>
#include <list>
#include <set>

#include <nowide/fstream.hpp>

#include "imgui_extras.hpp"
#include "utf8_strings.hpp"

const std::string recent_files_file = "recent files.txt";

void Toolbox::pushNewRecentFile(std::filesystem::path file, std::filesystem::path settings) {
    std::filesystem::create_directory(settings);
    auto recent_files_path = settings / recent_files_file;
    nowide::ifstream readFile(path_to_utf8_encoded_string(recent_files_path));
    std::list<std::string> recent;
    std::set<std::string> recent_set;
    for (std::string line; getline(readFile, line);) {
        if (recent_set.find(line) == recent_set.end()) {
            recent.push_back(line);
            recent_set.insert(line);
        }
    }
    readFile.close();

    recent.remove(path_to_utf8_encoded_string(file));

    while (recent.size() >= 10) {
        recent.pop_back();
    }

    recent.push_front(path_to_utf8_encoded_string(file));

    nowide::ofstream writeFile(
        path_to_utf8_encoded_string(recent_files_path),
        std::ofstream::out | std::ofstream::trunc
    );
    for (const auto& line : recent) {
        writeFile << line << std::endl;
    }
    writeFile.close();
}

std::vector<std::string> Toolbox::getRecentFiles(std::filesystem::path settings) {
    nowide::ifstream readFile{path_to_utf8_encoded_string(settings / recent_files_file)};
    std::vector<std::string> recent;
    for (std::string line; getline(readFile, line);) {
        recent.push_back(line);
    }
    readFile.close();
    return recent;
}
/*
 * return an sf::Time as ±MM:SS.mmm in a string
 */
std::string Toolbox::to_string(sf::Time time) {
    int minutes = static_cast<int>(std::abs(time.asSeconds())) / 60;
    int seconds = static_cast<int>(std::abs(time.asSeconds())) % 60;
    int miliseconds = static_cast<int>(std::abs(time.asMilliseconds())) % 1000;
    return fmt::format(
        "{}{:02}:{:02}.{:03}",
        time.asSeconds() < 0 ? "-" : "+",
        minutes,
        seconds,
        miliseconds
    );
}

float Toolbox::convertVolumeToNormalizedDB(int input) {
    auto vol = std::clamp(static_cast<float>(input) / 10.f, 0.f, 1.f);
    const auto b = 10.f;
    return (std::pow(b, vol) - 1.f) / (b - 1.f);
}

int Toolbox::getNextDivisor(int number, int starting_point) {
    if (number <= 0 or starting_point <= 0 or starting_point >= number) {
        return 1;
    } else {
        do {
            starting_point++;
        } while (number % starting_point != 0);
    }

    return starting_point;
}

int Toolbox::getPreviousDivisor(int number, int starting_point) {
    if (number <= 0 or starting_point <= 1 or starting_point > number) {
        return number;
    } else {
        do {
            starting_point--;
        } while (number % starting_point != 0);
    }

    return starting_point;
}

std::string Toolbox::toOrdinal(int number) {
    std::ostringstream s;
    s << number;
    // Special case : is it a xx1x ?
    if (number % 100 / 10 == 1) {
        s << "th";
    } else {
        switch (number % 10) {
            case 1:
                s << "st";
                break;
            case 2:
                s << "nd";
                break;
            case 3:
                s << "rd";
                break;
            default:
                s << "th";
                break;
        }
    }
    return s.str();
}
