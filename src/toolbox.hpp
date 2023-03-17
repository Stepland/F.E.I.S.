#pragma once

#include <filesystem>
#include <functional>
#include <optional>

#include <fmt/core.h>
#include <imgui.h>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include "imgui_extras.hpp"

/*
 * I just dump things here when I'm unsure whether they deserve a special file
 * for them or not
 */
namespace Toolbox {
    void pushNewRecentFile(std::filesystem::path file, std::filesystem::path settings);
    std::vector<std::string> getRecentFiles(std::filesystem::path settings);

    struct CustomConstraints {
        static void ContentSquare(ImGuiSizeCallbackData* data) {
            float TitlebarHeight =
                ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            float y = data->DesiredSize.y - TitlebarHeight;
            float x = data->DesiredSize.x;
            data->DesiredSize = ImVec2(std::max(x, y), std::max(x, y) + TitlebarHeight);
        }
    };

    std::string to_string(sf::Time time);
    float convertVolumeToNormalizedDB(int);
    int getNextDivisor(int number, int starting_point);
    int getPreviousDivisor(int number, int starting_point);
    std::string toOrdinal(int number);

    template<class T>
    bool edit_fill_color(const char* label, T& thing) {
        sf::Color col = thing.getFillColor();
        if (feis::ColorEdit4(label, col)) {
            thing.setFillColor(col);
            return true;
        }
        return false;
    }

    template<class T>
    void set_origin_normalized(T& s, float x, float y) {
        auto bounds = s.getGlobalBounds();
        s.setOrigin(bounds.left+x*bounds.width, bounds.top+y*bounds.height);
    }

    template<class T>
    void set_local_origin_normalized(T& s, float x, float y) {
        auto bounds = s.getLocalBounds();
        s.setOrigin(bounds.left+x*bounds.width, bounds.top+y*bounds.height);
    }

    template<class T>
    void center(T& thing) {
        set_local_origin_normalized(thing, 0.5, 0.5);
    }

    template<class T>
    sf::Vector2<T> top_left_given_normalized_anchor(
        const sf::Vector2<T>& pos,
        const sf::Vector2<T>& size,
        const sf::Vector2f origin
    ) {
        const sf::Vector2<T> offset = {size.x * origin.x, size.y * origin.y};
        return pos - offset;
    }

    template<class T>
    sf::Vector2<T> bottom_left_given_normalized_anchor(
        const sf::Vector2<T>& pos,
        const sf::Vector2<T>& size,
        const sf::Vector2f origin
    ) {
        return top_left_given_normalized_anchor(pos, size, origin) + sf::Vector2<T>{0, size.y};
    }
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
    T transform(T val) const { return a * val + b; };
    T clampedTransform(T val) const {
        return transform(std::clamp(val, low_input, high_input));
    };
    T backwards_transform(T val) const {
        return (val - b) / a;
    };

private:
    T a;
    T b;
    T low_input;
    T high_input;
    T low_output;
    T high_output;
};


template <class T>
struct fmt::formatter<std::optional<T>>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const std::optional<T>& opt, FormatContext& ctx) {
        if (opt) {
            return format_to(ctx.out(), "{}", *opt);
        } else {
            return format_to(ctx.out(), "∅");
        }
    }
};

template <class T>
struct fmt::formatter<std::shared_ptr<T>>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const std::shared_ptr<T>& ptr, FormatContext& ctx) {
        if (ptr) {
            return format_to(ctx.out(), "{}", *ptr);
        } else {
            return format_to(ctx.out(), "nullptr");
        }
    }
};