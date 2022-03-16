
#pragma once

/*
Dark template magic from https://www.modernescpp.com/index.php/visiting-a-std-variant-with-the-overload-pattern

Usage : 

    std::variant<char, int, float> var = 2017;

    auto TypeOfIntegral = VariantVisitor {
        [](char) { return "char"; },
        [](int) { return "int"; },
        [](auto) { return "unknown type"; },
    };

    std::visit(TypeOfIntegral, var);

*/
template<typename ... Ts>
struct VariantVisitor : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> VariantVisitor(Ts...) -> VariantVisitor<Ts...>;