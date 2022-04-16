#include <fmt/core.h>
#include <json.hpp>
#include <rapidcheck.h>
#include <rapidcheck/Check.h>
#include <rapidcheck/Classify.h>
#include <rapidcheck/Log.h>

#include "generators.hpp"

#include "../../better_note.hpp"
#include "../../better_notes.hpp"
#include "../../better_timing.hpp"
#include "../../json_decimal_handling.hpp"

int main() {
    rc::check(
        "A Decimal number literal can properly be read from json",
        [](const Decimal& d) {
            const auto json_string = fmt::format("{{\"a\": {}}}", d);
            const auto j = load_json_preserving_decimals(json_string);
            RC_ASSERT(load_as_decimal(j["a"]) == d);
        }
    );

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

    rc::check(
        "A Song object survives being converted to json and back",
        [](const better::Song& original) {
            const auto j = original.dump_to_memon_1_0_0();
            RC_LOG("json dump : "+j.dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore));
            const auto recovered = better::Song::load_from_memon_1_0_0(j);
            RC_ASSERT(original == recovered);
        }
    );
    return 0;
}