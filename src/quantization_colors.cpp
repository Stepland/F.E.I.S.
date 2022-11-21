#include "quantization_colors.hpp"
#include "color.hpp"
#include "toml++/impl/forward_declarations.h"
#include "variant_visitor.hpp"

namespace linear_view {
    sf::Color QuantizationColors::color_at_beat(const Fraction& time) {
        const auto denominator = time.denominator();
        if (denominator > palette.rbegin()->first) {
            return default_;
        }
        const auto& it = palette.find(static_cast<unsigned int>(denominator.get_ui()));
        if (it == palette.end()) {
            return default_;
        }
        return it->second;
    }

    void QuantizationColors::load_from_v1_0_0_table(const toml::table& linear_view_table) {
        const auto quant_colors_node = linear_view_table["quantization_colors"];
        const auto palette_node = quant_colors_node["palette"];
        if (const toml::array* arr = palette_node.as_array()) {
            std::map<unsigned int, sf::Color> new_palette;
            const auto parse_pairs = VariantVisitor {
                [&](const toml::array& pair){
                    if (pair.size() != 2) {
                        return;
                    }
                    const auto quant = pair[0].value<unsigned int>();
                    const auto color = parse_color(pair[1]);
                    if (quant and color) {
                        new_palette.emplace(*quant, *color);
                    }
                },
                [&](const auto&){}
            };
            arr->for_each(parse_pairs);
            if (not new_palette.empty()) {
                palette = std::move(new_palette);
            }
        }
        load_color(quant_colors_node["default"], default_);
    }

    void QuantizationColors::dump_as_v1_0_0(toml::table& linear_view_table) {
        toml::array palette_node;
        for (const auto& [quant, color] : palette) {
            palette_node.emplace_back<toml::array>(quant, dump_color(color));
        }
        toml::table quantization_colors_table{
            {"palette", palette_node},
            {"default", dump_color(default_)}
        };
        linear_view_table.insert_or_assign("quantization_colors", quantization_colors_table); 
    }
}
