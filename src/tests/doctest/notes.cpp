#include <doctest.h>

#include "../../better_notes.hpp"


TEST_CASE("better::Notes") {
    SUBCASE("can be copied on a subrange using .between()") {
        better::Notes original;
        original.insert(better::TapNote{0, {0,0}});
        original.insert(better::TapNote{1, {1,0}});
        original.insert(better::TapNote{2, {2,0}});
        original.insert(better::TapNote{3, {3,0}});

        better::Notes reference_copy;
        reference_copy.insert(better::TapNote{1, {1,0}});
        reference_copy.insert(better::TapNote{2, {2,0}});

        better::Notes copy = original.between({1,2});

        CHECK(copy == reference_copy);
        CHECK(copy.begin()->second == better::TapNote{1, {1,0}});
        CHECK(copy.rbegin()->second == better::TapNote{2, {2,0}});
    }

    SUBCASE("a copy created with .between()") {
        better::Notes copy;

        better::Notes reference_copy;
        reference_copy.insert(better::TapNote{1, {1,0}});
        reference_copy.insert(better::TapNote{2, {2,0}});

        SUBCASE("survivres the original's destruction") {

            {
                better::Notes original;
                original.insert(better::TapNote{0, {0,0}});
                original.insert(better::TapNote{1, {1,0}});
                original.insert(better::TapNote{2, {2,0}});
                original.insert(better::TapNote{3, {3,0}});

                copy = original.between({1,2});
            }
            
            CHECK(copy == reference_copy);
            CHECK(copy.begin()->second == better::TapNote{1, {1,0}});
            CHECK(copy.rbegin()->second == better::TapNote{2, {2,0}});
        }

        SUBCASE("survivres being created in another scope") {
            better::Notes original;
            original.insert(better::TapNote{0, {0,0}});
            original.insert(better::TapNote{1, {1,0}});
            original.insert(better::TapNote{2, {2,0}});
            original.insert(better::TapNote{3, {3,0}});

            {
                copy = original.between({1,2});
            }
            
            CHECK(copy == reference_copy);
            CHECK(copy.begin()->second == better::TapNote{1, {1,0}});
            CHECK(copy.rbegin()->second == better::TapNote{2, {2,0}});
        }
    }

    SUBCASE("can be searched with .contains()") {
        better::Notes original;
        original.insert(better::TapNote{0, {0,0}});

        better::TapNote a = {0, {0,0}};
        better::TapNote b = {0, {1,0}};
        better::LongNote c = {0, {0,0}, 1, {0,1}};

        CHECK(original.contains(a));
        CHECK_FALSE(original.contains(b));
        CHECK_FALSE(original.contains(c));
    }

    SUBCASE("can be merged with another better::Notes") {
        better::Notes original;
        original.insert(better::TapNote{0, {0,0}});

        better::Notes merged;
        original.insert(better::TapNote{1, {0,0}});
        merged.merge(std::move(original));

        CHECK(merged.size() == 2);
        CHECK(merged.begin()->second == better::TapNote{0, {0,0}});
    }
}