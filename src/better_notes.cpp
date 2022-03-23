#include "better_notes.hpp"

namespace better {
    std::pair<Notes::iterator, bool> Notes::insert(const Note& note) {
        auto conflicting_note = interval_tree::find(note.get_time_bounds());
        if (conflicting_note != end()) {
            return {conflicting_note, false};
        } else {
            auto it = interval_tree::insert({note.get_time_bounds(), note});
            return {it, true};
        }
    };

    Notes::const_iterator Notes::find(const Note& note) const {
        auto conflicting_note = interval_tree::end();
        in(note.get_time_bounds(), [&](Notes::iterator it){
            if (it->second == note and conflicting_note == end()) {
                conflicting_note = it;
            }
        });
        return conflicting_note;
    }
}