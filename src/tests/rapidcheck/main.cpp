#include <rapidcheck.h>
#include <rapidcheck/Check.h>
#include <rapidcheck/Classify.h>
#include <rapidcheck/Log.h>

#include "generators.hpp"

#include "../../better_note.hpp"
#include "../../better_notes.hpp"
#include "../../better_timing.hpp"
#include "json.hpp"

int main() {


    rc::check(
        "A Note survives being converted to json and back",
        [](const better::Note& n) {
            const auto j = n.dump_to_memon_1_0_0();
            RC_LOG("json dump : "+j.dump());
            const auto n_recovered = better::Note::load_from_memon_1_0_0(j);
            RC_ASSERT(n_recovered == n);
        }
    );

    rc::check(
        "A set of Notes survives being converted to json and back",
        [](const better::Notes& original) {
            const auto j = original.dump_to_memon_1_0_0();
            RC_LOG("json dump : "+j.dump());
            const auto recovered = better::Notes::load_from_memon_1_0_0(j);
            RC_ASSERT(original == recovered);
        }
    );

    rc::check(
        "A Timing object survives being converted to json and back",
        [](const better::Timing& original) {
            const auto j = original.dump_to_memon_1_0_0();
            RC_LOG("json dump : "+j.dump());
            const auto recovered = better::Timing::load_from_memon_1_0_0(j);
            RC_ASSERT(original == recovered);
        }
    );

    rc::check(
        "A Chart object survives being converted to json and back",
        [](const better::Chart& original) {
            const auto fallback_timing = nlohmann::ordered_json::object();
            const auto j = original.dump_to_memon_1_0_0(fallback_timing);
            RC_LOG("json dump : "+j.dump());
            const auto recovered = better::Chart::load_from_memon_1_0_0(j, fallback_timing);
            RC_ASSERT(original == recovered);
        }
    );
    return 0;
}