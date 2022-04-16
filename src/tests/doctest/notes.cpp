#include <doctest.h>

#include "../../better_notes.hpp"


TEST_CASE("better::Notes") {
    better::Notes original;
    original.insert(better::TapNote{0, {0,0}});
    original.insert(better::TapNote{1, {1,0}});
    original.insert(better::TapNote{2, {2,0}});
    original.insert(better::TapNote{3, {3,0}});
    better::Notes reference_copy;
    reference_copy.insert(better::TapNote{1, {1,0}});
    reference_copy.insert(better::TapNote{2, {2,0}});
    SUBCASE("can be copied on a subrange") {
        better::Notes copy = original.between({1,2});
        CHECK(copy == reference_copy);
        CHECK(copy.begin()->second == better::TapNote{1, {1,0}});
        CHECK(copy.rbegin()->second == better::TapNote{2, {2,0}});
        SUBCASE("the copy survives the destruction of the original") {
            original.interval_tree::~interval_tree();
            CHECK(copy == reference_copy);
            CHECK(copy.begin()->second == better::TapNote{1, {1,0}});
            CHECK(copy.rbegin()->second == better::TapNote{2, {2,0}});
        }
    }
}