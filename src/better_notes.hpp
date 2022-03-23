#pragma once

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

#include "interval_tree.hpp"

#include "better_note.hpp"
#include "special_numeric_types.hpp"

namespace better {
    class Notes: public interval_tree<Fraction, Note> {
    public:
        std::pair<iterator, bool> insert(const Note& note);
        const_iterator find(const Note& note) const;
    };
}