#include "json_decimal_parser.hpp"

bool json_decimal_parser::number_float(number_float_t /*unused*/, const string_t& val) {
    string_t copy = val;
    string(copy);
    return true;
}