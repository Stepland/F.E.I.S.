#include "toolbox.hpp"
#include "imgui/custom.hpp"

#include <cmath>
#include <fstream>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iomanip>
#include <list>
#include <set>

const std::string recent_files_file = "recent files.txt";

void Toolbox::pushNewRecentFile(std::filesystem::path file, std::filesystem::path settings) {
    std::filesystem::create_directory(settings);
    auto recent_files_path = settings / recent_files_file;
    std::ifstream readFile(recent_files_path);
    std::list<std::string> recent;
    std::set<std::string> recent_set;
    for (std::string line; getline(readFile, line);) {
        if (recent_set.find(line) == recent_set.end()) {
            recent.push_back(line);
            recent_set.insert(line);
        }
    }
    readFile.close();

    recent.remove(std::filesystem::canonical(file).string());

    while (recent.size() >= 10) {
        recent.pop_back();
    }

    recent.push_front(std::filesystem::canonical(file).string());

    std::ofstream writeFile(
        recent_files_path,
        std::ofstream::out | std::ofstream::trunc
    );
    for (const auto& line : recent) {
        writeFile << line << std::endl;
    }
    writeFile.close();
}

std::vector<std::string> Toolbox::getRecentFiles(std::filesystem::path settings) {
    std::ifstream readFile{settings / recent_files_file};
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
    std::ostringstream stringStream;
    int minutes = static_cast<int>(std::abs(time.asSeconds())) / 60;
    int seconds = static_cast<int>(std::abs(time.asSeconds())) % 60;
    int miliseconds = static_cast<int>(std::abs(time.asMilliseconds())) % 1000;
    if (time.asSeconds() < 0) {
        stringStream << "-";
    } else {
        stringStream << "+";
    }
    stringStream.fill('0');
    stringStream << std::setw(2) << minutes << ":" << std::setw(2) << seconds
                 << "." << std::setw(3) << miliseconds;
    //("-%02d:%02d.%03d",minutes,seconds,miliseconds);
    return stringStream.str();
}

/*
 * Imgui::InputText that gets colored Red when isValid is false and
 * hoverTextHelp gets displayed when hovering over invalid input When input is
 * valid InputText gets colored green Displays InputText without any style
 * change if the input is empty;
 */
bool Toolbox::InputTextColored(
    const char* label,
    std::string* str,
    bool isValid,
    const std::string& hoverHelpText
) {
    bool return_value;
    if (str->empty()) {
        return ImGui::InputText(label, str);
    } else {
        Toolbox::CustomColors colors;
        if (not isValid) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.FrameBg_Red.Value);
            ImGui::PushStyleColor(
                ImGuiCol_FrameBgHovered,
                colors.FrameBgHovered_Red.Value);
            ImGui::PushStyleColor(
                ImGuiCol_FrameBgActive,
                colors.FrameBgActive_Red.Value);
        } else {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.FrameBg_Green.Value);
            ImGui::PushStyleColor(
                ImGuiCol_FrameBgHovered,
                colors.FrameBgHovered_Green.Value);
            ImGui::PushStyleColor(
                ImGuiCol_FrameBgActive,
                colors.FrameBgActive_Green.Value);
        }
        return_value = ImGui::InputText(label, str);
        if (ImGui::IsItemHovered() and (not isValid)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(hoverHelpText.c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopStyleColor(3);
        return return_value;
    }
}

float Toolbox::convertVolumeToNormalizedDB(int input) {
    auto vol = std::clamp(static_cast<float>(input) / 10.f, 0.f, 1.f);
    const auto b = 10.f;
    return (std::pow(b, vol) - 1.f) / (b - 1.f);
}

int Toolbox::getNextDivisor(int number, int starting_point) {
    assert(number > 0);
    assert(starting_point > 0 and starting_point <= number);

    if (starting_point == number) {
        return 1;
    } else {
        do {
            starting_point++;
        } while (number % starting_point != 0);
    }

    return starting_point;
}

int Toolbox::getPreviousDivisor(int number, int starting_point) {
    assert(number > 0);
    assert(starting_point > 0 and starting_point <= number);

    if (starting_point == 1) {
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

void Toolbox::center(sf::Shape& s) {
    sf::FloatRect bounds = s.getLocalBounds();
    s.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
}

bool Toolbox::editFillColor(const char* label, sf::Shape& s) {
    sf::Color col = s.getFillColor();
    if (ImGui::ColorEdit4(label, col)) {
        s.setFillColor(col);
        return true;
    }
    return false;
}
