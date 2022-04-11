#include <rapidcheck.h>

#include "generators.hpp"

#include "../../better_note.hpp"

int main() {
    rc::check(
        "A Note survives being converted to json and back",
        [](const better::Note& n) {
            const auto j = n.dump_to_memon_1_0_0();
            const auto n_recovered = better::Note::load_from_memon_1_0_0(j);
            RC_ASSERT(n_recovered == n);
        }
    );

    rc::check(
        "A set of Notes survive being converted to json and back",
        [](const better::Notes& ns) {
            const auto j = ns.dump_to_memon_1_0_0();
            RC_TAG(j.dump());
            const auto n_recovered = better::Notes::load_from_memon_1_0_0(j);
            RC_ASSERT(n_recovered == ns);
        }
    );
    return 0;
}