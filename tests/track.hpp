#include <any>
#include <map>
#include <iostream>
#include <optional>
#include <string>

static std::map<std::string, std::any> old_values;

template<class T>
void track(const std::string& name, const T& value) {
    std::optional<T> old_value;
    auto old_value_it = old_values.find(name);
    if (old_value_it != old_values.end()) {
        old_value = std::any_cast<T>(old_value_it->second);
    }

    if (old_value != value) {
        old_values[name] = value;
        std::cout << name << " = " << value << std::endl;
    }
}