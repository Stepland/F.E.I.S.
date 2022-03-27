#include "notes_clipboard.hpp"
#include <utility>

#include "src/better_note.hpp"
#include "variant_visitor.hpp"

void NotesClipboard::copy(const better::Notes& notes) {
    contents.clear();
    if (not notes.empty()) {
        const auto offset = notes.cbegin()->second.get_time();
        const auto shift = shifter(-offset);
        for (const auto& [_, note] : notes) {
            contents.insert(note.visit(shift));
        }
    }
}

better::Notes NotesClipboard::paste(Fraction offset) {
    better::Notes res;
    const auto shift = shifter(offset);
    for (const auto& [_, note] : contents) {
        res.insert(note.visit(shift));
    }
    return res;
}
