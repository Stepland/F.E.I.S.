#pragma once

#include <optional>
#include <sstream>
#include <string>

template<class A, class B>
B apply_or(std::optional<A> opt, B(*func)(A), B b) {
    if (opt.has_value()) {
        return func(*opt);
    } else {
        return b;
    }
}

template<class A>
std::string stringify_or(std::optional<A> opt, std::string fallback) {
    return apply_or(
        opt,
        [](const A& a){
            std::stringstream ss;
            ss << a;
            return ss.str();
        },
        fallback
    )
}