#include "notes_clipboard.hpp"

NotesClipboard::NotesClipboard(const std::set<Note> &notes) {
    copy(notes);
}

void NotesClipboard::copy(const std::set<Note> &notes) {
    contents.clear();
    if (not notes.empty()) {
        int timing_offset = notes.cbegin()->getTiming();
        for (const auto &note : notes) {
            contents.emplace(note.getPos(), note.getTiming()-timing_offset, note.getLength(), note.getTail_pos());
        }
    }
}

std::set<Note> NotesClipboard::paste(int tick_offset) {
    std::set<Note> res = {};
    for (auto note : contents) {
        res.emplace(note.getPos(), note.getTiming()+tick_offset, note.getLength(), note.getTail_pos());
    }
    return res;
}
