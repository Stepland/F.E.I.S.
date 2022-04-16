#pragma once

#include <vector>
#include <utility>

#include "better_note.hpp"
#include "better_notes.hpp"
#include "special_numeric_types.hpp"
#include "variant_visitor.hpp"

/*
Stores a vector of notes with times relative to the first note in the vector,
to allow pasting notes at another time in the chart by simply shifting
all the note starting times.
*/
class NotesClipboard {
public:
    NotesClipboard() = default;
    void copy(const better::Notes& notes);
    better::Notes paste(Fraction beat);

    bool empty() { return contents.empty(); };

private:
    better::Notes contents = {};
};

const auto shifter = [](Fraction offset){
    return VariantVisitor {
        [=](const better::TapNote& tap_note) {
            return better::Note(
                std::in_place_type<better::TapNote>,
                tap_note.get_time() + offset,
                tap_note.get_position()
            );
        },
        [=](const better::LongNote& long_note) {
            return better::Note(
                std::in_place_type<better::LongNote>,
                long_note.get_time() + offset,
                long_note.get_position(),
                long_note.get_duration(),
                long_note.get_tail_tip()
            );
        },
    };
};


