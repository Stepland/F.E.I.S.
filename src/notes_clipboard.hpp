#ifndef FEIS_NOTESCLIPBOARD_H
#define FEIS_NOTESCLIPBOARD_H

#include <set>

#include "note.hpp"

class NotesClipboard {
public:
    explicit NotesClipboard(const std::set<Note>& notes = {});

    void copy(const std::set<Note>& notes);
    std::set<Note> paste(int tick_offset);

    bool empty() { return contents.empty(); };

private:
    std::set<Note> contents;
};

#endif  // FEIS_NOTESCLIPBOARD_H