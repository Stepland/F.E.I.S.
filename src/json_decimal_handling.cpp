#include "json_decimal_handling.hpp"
#include <stdexcept>
#include "special_numeric_types.hpp"

bool json_decimal_parser::number_float(number_float_t /*unused*/, const string_t& val) {
    string_t copy = val;
    string(copy);
    return true;
}

Decimal load_as_decimal(const nlohmann::json& j) {
    if (j.is_string()) {
        return Decimal{j.get<std::string>()};
    } else if (j.is_number_unsigned()) {
        return Decimal{j.get<std::uint64_t>()};
    } else if (j.is_number_integer()) {
        return Decimal{j.get<std::int64_t>()};
    } else {
        throw std::invalid_argument(fmt::format(
            "Found a json value of unexpected kind when trying to read"
            "a decimal number : {} (json type: {})",
            j.dump(),
            j.type_name()
        ));
    }
}