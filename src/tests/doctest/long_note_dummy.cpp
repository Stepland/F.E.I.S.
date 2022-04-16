#include <doctest.h>

#include "../../long_note_dummy.hpp"


TEST_CASE("make_long_note works with any input") {
    for (unsigned int index_a = 0; index_a < 16; index_a++) {
        for (unsigned int index_b = 0; index_b < 16; index_b++) {
            better::Position pos_a{index_a};
            better::Position pos_b{index_b};
            better::TapNote a{0, pos_a};
            better::TapNote b{0, pos_b};
            REQUIRE_NOTHROW(make_linear_view_long_note_dummy({a, b}, 1));
        }
    }
}