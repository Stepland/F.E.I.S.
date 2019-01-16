//
// Created by Syméon on 13/01/2019.
//

#include <imgui.h>
#include <imgui_stdlib.h>
#include <list>
#include <set>
#include <cmath>
#include <c++/8.2.1/fstream>
#include "Toolbox.h"

void Toolbox::pushNewRecentFile(std::filesystem::path path) {
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
 * InputText that gets colored Red when isValid is false and hoverTextHelp gets displayed when hovering over invalid input
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
