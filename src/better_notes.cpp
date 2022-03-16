#include "better_notes.hpp"

namespace better {
    std::pair<Notes::iterator, bool> Notes::insert(const Note& note) {
        std::optional<Notes::iterator> conflicting_note;
        in(
            note.get_time_bounds(),
            [&](iterator it){
                if (it->second.get_position() == note.get_position()) {
                    if (not conflicting_note.has_value()) {
                        conflicting_note = it;
                    }
                }
            }
        );
        if (conflicting_note.has_value()) {
            return {*conflicting_note, false};
        } else {
            auto it = interval_tree::insert({note.get_time_bounds(), note});
            return {it, true};
        }
    };
}