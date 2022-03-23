#pragma once

#define IM_MAX(_A, _B) (((_A) >= (_B)) ? (_A) : (_B))

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Time.hpp>
#include <filesystem>
#include <functional>
#include <imgui.h>

/*
 * I just dump things here when I'm unsure whether they deserve a special file
 * for them or not
 */
namespace Toolbox {

    struct CustomColors {
        ImColor FrameBg_Green = {0.163f, 0.480f, 0.160f, 0.540f};
        ImColor FrameBgHovered_Green = {0.261f, 0.980f, 0.261f, 0.400f};
        ImColor FrameBgActive_Green = {0.261f, 0.980f, 0.261f, 0.671f};
        ImColor FrameBg_Red = {0.480f, 0.160f, 0.160f, 0.540f};
        ImColor FrameBgHovered_Red = {0.980f, 0.261f, 0.261f, 0.400f};
        ImColor FrameBgActive_Red = {0.980f, 0.261f, 0.261f, 0.671f};
    };
    void pushNewRecentFile(std::filesystem::path file, std::filesystem::path settings);
    std::vector<std::string> getRecentFiles(std::filesystem::path settings);

    struct CustomConstraints {
        static void ContentSquare(ImGuiSizeCallbackData* data) {
            float TitlebarHeight =
                ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            float y = data->DesiredSize.y - TitlebarHeight;
            float x = data->DesiredSize.x;
            data->DesiredSize = ImVec2(IM_MAX(x, y), IM_MAX(x, y) + TitlebarHeight);
        }
    };

    std::string to_string(sf::Time time);
    bool InputTextColored(const char* label, std::string* str, bool isValid, const std::string& hoverHelpText);
    float convertVolumeToNormalizedDB(int);
    int getNextDivisor(int number, int starting_point);
    int getPreviousDivisor(int number, int starting_point);
    std::string toOrdinal(int number);
    void center(sf::Shape& s);
    bool editFillColor(const char* label, sf::Shape& s);
}

template<typename T>
class AffineTransform {
public:
    AffineTransform(T low_input, T high_input, T low_output, T high_output) :
        low_input(low_input),
        high_input(high_input),
        low_output(low_output),
        high_output(high_output) {
        if (low_input == high_input) {
            throw std::invalid_argument(
                "low and high input values for affine transform must be "
                "different !");
        }
        a = (high_output - low_output) / (high_input - low_input);
        b = (high_input * low_output - high_output * low_input) / (high_input - low_input);
    };
    T transform(T val) { return a * val + b; };
    T clampedTransform(T val) {
        return transform(std::clamp(val, low_input, high_input));
    };
    T backwards_transform(T val) {
        // if we're too close to zero
        if (std::abs(a) < 10e-10) {
            throw std::runtime_error(
                "Can't apply backwards transformation, coefficient is too "
                "close to zero");
        } else {
            return (val - b) / a;
        }
    };

private:
    T a;
    T b;

public:
    void setB(T b) { AffineTransform::b = b; }

private:
    T low_input;
    T high_input;
    T low_output;
    T high_output;
};
