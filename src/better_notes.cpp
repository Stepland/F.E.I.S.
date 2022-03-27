#include "better_notes.hpp"

namespace better {
    std::pair<Notes::iterator, bool> Notes::insert(const Note& note) {
        auto conflicting_note = end();
        in(
            note.get_time_bounds(),
            [&](Notes::iterator& it){
                if (
                    it->second.get_position() == note.get_position()
                    and conflicting_note == end()
                ) {
                    conflicting_note = it;
                }
            }
        );
        if (conflicting_note != end()) {
            return {conflicting_note, false};
        } else {
            auto it = interval_tree::insert({note.get_time_bounds(), note});
            return {it, true};
        }
    };

    void Notes::overwriting_insert(const Note& note) {
        std::vector<better::Note> conflicting_notes = {};
        in(
            note.get_time_bounds(),
            [&](Notes::iterator& it) {
                if (it->second.get_position() == note.get_position()) {
                    conflicting_notes.push_back(it->second);
                }
            }
        );
        for (const auto& conflict : conflicting_notes) {
            erase(conflict);
        }
        interval_tree::insert({note.get_time_bounds(), note});
    };

    Notes::const_iterator Notes::find(const Note& note) const {
        auto conflicting_note = interval_tree::end();
        in(note.get_time_bounds(), [&](Notes::iterator& it){
            if (it->second == note and conflicting_note == end()) {
                conflicting_note = it;
            }
        });
        return conflicting_note;
    };

    bool Notes::contains(const Note& note) const {
        return find(note) != cend();
    };

    void Notes::erase(const Note& note) {
        auto it = find(note);
        interval_tree::erase(it);
    };
}