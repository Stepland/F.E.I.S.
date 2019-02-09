//
// Created by Syméon on 13/01/2019.
//

#ifndef FEIS_TOOLBOX_H
#define FEIS_TOOLBOX_H

#define IM_MAX(_A,_B)       (((_A) >= (_B)) ? (_A) : (_B))

#include <SFML/Audio.hpp>
#include <functional>
#include <filesystem>
#include <SFML/Graphics/Texture.hpp>
#include <imgui-SFML.h>

namespace Toolbox {

    struct CustomColors {
        ImColor FrameBg_Green = {0.163f, 0.480f, 0.160f, 0.540f};
        ImColor FrameBgHovered_Green = {0.261f, 0.980f, 0.261f, 0.400f};
        ImColor FrameBgActive_Green = {0.261f, 0.980f, 0.261f, 0.671f};
        ImColor FrameBg_Red = {0.480f, 0.160f, 0.160f, 0.540f};
        ImColor FrameBgHovered_Red = {0.980f, 0.261f, 0.261f, 0.400f};
        ImColor FrameBgActive_Red = {0.980f, 0.261f, 0.261f, 0.671f};
    };
    void pushNewRecentFile(std::filesystem::path path);
    std::vector<std::string> getRecentFiles();

    struct CustomConstraints {
        static void ContentSquare(ImGuiSizeCallbackData* data) {
            float TitlebarHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            float y = data->DesiredSize.y - TitlebarHeight;
            float x = data->DesiredSize.x;
            data->DesiredSize = ImVec2(IM_MAX(x,y), IM_MAX(x,y) + TitlebarHeight);
        }
    };
    std::string to_string(sf::Time time);
    bool InputTextColored(bool isValid, const std::string& hoverHelpText, const char *label, std::string *str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
    float convertToLogarithmicVolume(int);
    void updateVolume(sf::SoundSource& soundSource,int volume);
    int getNextDivisor(int number, int starting_point);
    int getPreviousDivisor(int number, int starting_point);
    std::string toOrdinal(int number);

    void displayIfHasValue(const std::optional<std::reference_wrapper<sf::Texture>>& tex, ImVec2 cursorPosition, ImVec2 texSize, int& index);
}

template<typename T>
class AffineTransform {
public:
    AffineTransform(T low_input, T high_input, T low_output, T high_output):
        low_input(low_input),
        high_input(high_input),
        low_output(low_output),
        high_output(high_output)
        {
        if (low_input == high_input) {
            throw std::invalid_argument("low and high input values for affine transform must be different !");
        }
        a = (high_output-low_output)/(high_input-low_input);
        b = (high_input*low_output - high_output*low_input)/(high_input-low_input);
    };
    T transform(T val) {return a*val + b;};
    T clampedTransform(T val) { return transform(std::clamp(val,low_input,high_input));};
    T backwards_transform(T val) {
        // if we're too close to zero
        if (std::abs(a) < 10e-10) {
            throw std::runtime_error("Can't apply backwards transformation, coefficient is too close to zero");
        } else {
            return (val-b)/a;
        }
    };
private:
    T a;
    T b;

    T low_input;
    T high_input;
    T low_output;
    T high_output;
};

#endif //FEIS_TOOLBOX_H
