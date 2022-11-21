#include "lane_order.hpp"

#include <sstream>

#include "../variant_visitor.hpp"


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

namespace linear_view::lane_order {
    LaneOrder load_from_v1_0_0_table(const toml::table& linear_view) {
        auto lane_order = linear_view::default_lane_order;
        const auto lane_order_node = linear_view["lane_order"];
        const auto type = lane_order_node["type"].value<std::string>();
        if (not type) {
            return lane_order;
        }
        
        if (*type == "default") {
            return Default{};
        } else if (*type == "vertical") {
            return Vertical{};
        } else if (*type == "custom") {
            const auto order_as_string = lane_order_node["order"].value<std::string>();
            if (order_as_string) {
                return Custom{*order_as_string};
            }
        }
        
        return lane_order;
    }

    void dump_as_v1_0_0(const LaneOrder& lane_order, toml::table& linear_view) {
        const auto _dump = VariantVisitor {
            [&](const Default&) {
                linear_view.insert_or_assign(
                    "lane_order",
                    toml::table{{"type", "default"}}
                );
            },
            [&](const Vertical&) {
                linear_view.insert_or_assign(
                    "lane_order",
                    toml::table{{"type", "vertical"}}
                );
            },
            [&](const Custom& custom) {
                linear_view.insert_or_assign(
                    "lane_order",
                    toml::table{
                        {"type", "custom"},
                        {"order", custom.as_string}
                    }
                );
            }
        };
        std::visit(_dump, lane_order);
    }
}