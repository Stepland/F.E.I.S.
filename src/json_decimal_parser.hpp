#pragma once

#include <json.hpp>
/*
This class allows parsing json files with nlohmann::json while losslessly
recovering decimal number literals by storing their original string
representation when parsing intead of their float or double conversion

Usage :

    nlohmann::json j;
    json_decimal_parser sax{j};
    nlohmann::json::sax_parse(..., &sax);

*/

using sax_parser = nlohmann::detail::json_sax_dom_parser<nlohmann::json>;

class json_decimal_parser : public sax_parser {
public:
    /*
    Inherit the constructor because life is too short to write constructors for
    derived classes
    */
    using sax_parser::json_sax_dom_parser;

    // override float parsing, divert it to parsing the original string
    // as a json string literal
    bool number_float(number_float_t /*unused*/, const string_t& val);
};

template<class InputType>
nlohmann::json parse_decimal_json(InputType&& input) {
    nlohmann::json j;
    json_decimal_parser sax{j};
    nlohmann::json::sax_parse(std::forward<InputType>(input), &sax);
    return j;
}