#include "lane_order.hpp"

#include <sstream>


linear_view::lane_order::Custom::Custom() :
    lane_to_button({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15})
{
    update_from_array();
}

linear_view::lane_order::Custom::Custom(const std::string& as_string_) :
    as_string(as_string_)
{
    cleanup_string();
    update_from_string();
}

void linear_view::lane_order::Custom::update_from_array() {
    std::stringstream ss;
    button_to_lane.clear();
    for (std::size_t lane = 0; lane < lane_to_button.size(); lane++) {
        const auto button = lane_to_button.at(lane);
        if (button) {
            ss << letters.at(*button);
            button_to_lane[*button] = lane;
        } else {
            ss << "_";
        }
    }
    as_string = ss.str();
}

void linear_view::lane_order::Custom::cleanup_string() {
    as_string.resize(16);
    for (auto& c : as_string) {
        if (not letter_to_index.contains(c)) {
            c = '_';
        }
    }
}

void linear_view::lane_order::Custom::update_from_string() {
    lane_to_button = {{
        {}, {}, {}, {},
        {}, {}, {}, {},
        {}, {}, {}, {},
        {}, {}, {}, {}
    }};
    button_to_lane.clear();
    const auto upper_bound = std::min(16UL, as_string.length());
    for (std::size_t lane = 0; lane < upper_bound; lane++) {
        const auto letter = as_string.at(lane);
        const auto pair = letter_to_index.find(letter);
        if (pair != letter_to_index.end()) {
            const auto button = pair->second;
            lane_to_button[lane] = button;
            button_to_lane[button] = lane;
        }
    }
}