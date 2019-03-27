//
// Created by Syméon on 13/01/2019.
//

#include <imgui.h>
#include <imgui_stdlib.h>
#include <list>
#include <set>
#include <cmath>
#include <fstream>
#include <iomanip>
#include "Toolbox.h"

void Toolbox::pushNewRecentFile(std::filesystem::path path) {

    std::filesystem::create_directory("settings");
    std::ifstream readFile(std::filesystem::path("settings/recent files.txt"));
    std::list<std::string> recent;
    std::set<std::string> recent_set;
    for(std::string line; getline( readFile, line );) {
        if (recent_set.find(line) == recent_set.end()) {
            recent.push_back(line);
            recent_set.insert(line);
        }
    }
    readFile.close();

    recent.remove(std::filesystem::canonical(path).string());

    while (recent.size() >= 10) {
        recent.pop_back();
    }

    recent.push_front(std::filesystem::canonical(path).string());

    std::ofstream writeFile("settings/recent files.txt", std::ofstream::out | std::ofstream::trunc);
    for (const auto& line : recent) {
        writeFile << line << std::endl;
    }
    writeFile.close();
}

std::vector<std::string> Toolbox::getRecentFiles() {
    std::ifstream readFile(std::filesystem::path("settings/recent files.txt"));
    std::vector<std::string> recent;
    for(std::string line; getline( readFile, line ); ){
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
    int minutes = static_cast<int>(std::abs(time.asSeconds()))/60;
    int seconds = static_cast<int>(std::abs(time.asSeconds()))%60;
    int miliseconds = static_cast<int>(std::abs(time.asMilliseconds()))%1000;
    if (time.asSeconds() < 0) {
        stringStream << "-";
    } else {
        stringStream << "+";
    }
    stringStream.fill('0');
    stringStream << std::setw(2) << minutes << ":" << std::setw(2) << seconds << "." << std::setw(3) << miliseconds;
    //("-%02d:%02d.%03d",minutes,seconds,miliseconds);
    return stringStream.str();
}

/*
 * Imgui::InputText that gets colored Red when isValid is false and hoverTextHelp gets displayed when hovering over invalid input
 * When input is valid InputText gets colored green
 * Displays InputText without any style change if the input is empty;
 */
bool Toolbox::InputTextColored(bool isValid, const std::string& hoverHelpText, const char *label, std::string *str, ImGuiInputTextFlags flags,
                               ImGuiInputTextCallback callback, void *user_data) {
    bool return_value;
    if (str->empty()) {
        return_value = ImGui::InputText(label,str,flags,callback,user_data);
    } else {
        Toolbox::CustomColors colors;
        if (not isValid) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.FrameBg_Red.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors.FrameBgHovered_Red.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors.FrameBgActive_Red.Value);
        } else {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.FrameBg_Green.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors.FrameBgHovered_Green.Value);
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors.FrameBgActive_Green.Value);
        }
        return_value = ImGui::InputText(label,str,flags,callback,user_data);
        if (ImGui::IsItemHovered() and (not isValid)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(hoverHelpText.c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopStyleColor(3);
    }
    return return_value;
}

/*
 * Quick formula to get an exponential function of the integer volume setting mapping 0 to 0.f and 10 to 100.f
 */
float Toolbox::convertToLogarithmicVolume(int x) {
    if (x > 10) {
        return 100.f;
    } else if (x < 0) {
        return 0.f;
    }
    return static_cast<float>(pow(2.f, static_cast<float>(x)*log(101.f)/(10*log(2.f))) - 1.f);
}

void Toolbox::updateVolume(sf::SoundSource &soundSource, int volume) {
    soundSource.setVolume(Toolbox::convertToLogarithmicVolume(volume));
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
    if (number%100/10 == 1) {
        s << "th";
    } else {
        switch (number%10) {
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

void
Toolbox::displayIfHasValue(const std::optional<std::reference_wrapper<sf::Texture>> &tex, ImVec2 cursorPosition, ImVec2 texSize, int &index) {
    if (tex) {
        ImGui::SetCursorPos(cursorPosition);
        ImGui::PushID(index);
        ImGui::Image(*tex,texSize);
        ImGui::PopID();
        ++index;
    }
}

void Toolbox::center(sf::Shape &s) {
    sf::FloatRect bounds = s.getLocalBounds();
    s.setOrigin(bounds.left + bounds.width/2.f, bounds.top  + bounds.height/2.f);
}

bool Toolbox::editFillColor(const char* label, sf::Shape& s) {

    sf::Color col = s.getFillColor();
    if (ImGui::ColorEdit4(label, col)) {
        s.setFillColor(col);
        return true;
    }
    return false;
}
