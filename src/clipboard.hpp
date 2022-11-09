#pragma once

#include <vector>
#include <utility>

#include "better_note.hpp"
#include "better_notes.hpp"
#include "special_numeric_types.hpp"
#include "src/better_timing.hpp"
#include "variant_visitor.hpp"

struct ClipboardContents {
    better::Notes notes;
    std::set<better::BPMAtBeat, better::Timing::beat_order_for_events> bpm_events;

    ClipboardContents shifted_by(Fraction offset) const;
    bool empty() const;
    void clear();
};

/*
Stores a collection of notes with times relative to the first note in the vector,
to allow pasting notes at another time in the chart by simply shifting
all the note starting times.
*/
class Clipboard {
public:
    Clipboard() = default;
    void copy(const ClipboardContents& contents);
    ClipboardContents paste(Fraction beat) const;

    bool empty() const;
private:
    ClipboardContents contents;
};

