#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <interval_tree.hpp>
#include <json.hpp>

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
        // insert a note, erasing any other note it collides with,
        // returns the set of erased notes
        Notes overwriting_insert(const Note& note);

        // insert each note in other
        void merge(Notes&& other);

        // returns at iterator to a note exactly equal, if found
        const_iterator find(const Note& note) const;
        bool contains(const Note& note) const;
        void erase(const Note& note);

        /*
        Returns true if the given note (assumed to already be in the container)
        is colliding with ANOTHER note. This means notes exactly equal to the
        one passed as an argument are NOT taken into account.
        */
        bool is_colliding(const better::Note& note, const better::Timing& timing, const sf::Time& collision_zone) const;

        Notes between(const Interval<Fraction>& bounds);
        std::size_t count_between(const Interval<Fraction>& bounds);

        nlohmann::ordered_json dump_to_memon_1_0_0() const;

        static Notes load_from_memon_1_0_0(const nlohmann::json& json, std::uint64_t resolution = 240);
        static Notes load_from_memon_legacy(const nlohmann::json& json, std::uint64_t resolution);

        friend std::ostream& operator<<(std::ostream& out, const Notes& ns);
    };
}

template <>
struct fmt::formatter<better::Notes>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const better::Notes& n, FormatContext& ctx) {
        std::vector<better::Note> notes;
        for (const auto& [_, note] : n) {
            notes.push_back(note);
        }
        return format_to(
            ctx.out(),
            "[{}]",
            fmt::join(
                notes,
                ", "
            )
        );
    }
};