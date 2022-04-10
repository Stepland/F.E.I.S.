#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <interval_tree.hpp>
#include <json.hpp>
#include <type_traits>
#include <utility>

#include "better_note.hpp"
#include "better_timing.hpp"
#include "generic_interval.hpp"
#include "json.hpp"
#include "special_numeric_types.hpp"

namespace better {
    class Notes : public interval_tree<Fraction, Note> {
    public:
        // try to insert a note, the boolean is true on success
        std::pair<iterator, bool> insert(const Note& note);
        // insert a note, erasing any other note it collides with
        void overwriting_insert(const Note& note);
        // returns at iterator to a note exactly equal, if found
        const_iterator find(const Note& note) const;
        bool contains(const Note& note) const;
        void erase(const Note& note);

        /*
        Returns true if the given note (assumed to already be in the container)
        is colliding with ANOTHER note. This means notes exactly equal to the
        one passed as an argument are NOT taken into account.
        */
        bool is_colliding(const better::Note& note, const better::Timing& timing) const;

        Notes between(const Interval<Fraction>& bounds);
        std::size_t count_between(const Interval<Fraction>& bounds);

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Notes load_from_memon_1_0_0(const nlohmann::json& json, std::uint64_t resolution = 240);
        static Notes load_from_memon_legacy(const nlohmann::json& json, std::uint64_t resolution);
    };
}