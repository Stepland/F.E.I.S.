#include <iostream>
#include <map>
#include <cstdint>
#include <string>
#include <vector>

#include <json.hpp>
#include <libmpdec++/decimal.hh>

class json_decimal_parser : public nlohmann::detail::json_sax_dom_parser<nlohmann::json> {
public:
    using nlohmann::detail::json_sax_dom_parser<nlohmann::json>::json_sax_dom_parser;
    bool number_float(number_float_t /*unused*/, const string_t& val) {
        string_t copy = val;
        string(copy);
        return true;
    }
};

int main() {
    nlohmann::json j{"blablabla"};
    // json_decimal_parser sax{j};
    // std::string json;
    // std::getline(std::cin, json);
    // nlohmann::json::sax_parse(json, &sax);
    std::cout << "from const char * {} : " << nlohmann::json{"blablabla"} << std::endl;
    std::cout << "from string {} : " << nlohmann::json{std::string{"blibloblu"}} << std::endl;
    std::cout << "from const char * () : " << nlohmann::json("blablabla") << std::endl;
    std::cout << "from string {} : " << nlohmann::json(std::string("blibloblu")) << std::endl;
}