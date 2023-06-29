#include "clipboard.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <iterator>
#include <optional>
#include <utility>

#include "better_note.hpp"
#include "better_timing.hpp"
#include "variant_visitor.hpp"

NoteAndBPMSelection NoteAndBPMSelection::shifted_by(Fraction offset) const {
    NoteAndBPMSelection res;
    const auto shift = VariantVisitor {
        [&](const better::TapNote& tap_note) {
            return better::Note(
                std::in_place_type<better::TapNote>,
                tap_note.get_time() + offset,
                tap_note.get_position()
            );
        },
        [&](const better::LongNote& long_note) {
            return better::Note(
                std::in_place_type<better::LongNote>,
                long_note.get_time() + offset,
                long_note.get_position(),
                long_note.get_duration(),
                long_note.get_tail_tip()
            );
        },
    };
    for (auto& [_, note] : notes) {
        res.notes.insert(note.visit(shift));
    }

    for (const auto& event : bpm_events) {
        res.bpm_events.emplace(
            event.get_bpm(),
            event.get_beats() + offset
        );
    }
    return res;
}

bool NoteAndBPMSelection::empty() const {
    return notes.empty() and bpm_events.empty();
}

void NoteAndBPMSelection::clear() {
    notes.clear();
    bpm_events.clear();
}

void Clipboard::copy(const NoteAndBPMSelection& new_contents) {
    const auto offset = [&](){
        std::set<Fraction> offsets = {};
        if (not new_contents.notes.empty()) {
            offsets.insert(new_contents.notes.cbegin()->second.get_time());
        }
        if (not new_contents.bpm_events.empty()) {
            offsets.insert(new_contents.bpm_events.cbegin()->get_beats());
        }

        if (offsets.empty()) {
            return Fraction{0};
        } else {
            return *offsets.cbegin();
        }
    }();
    contents = new_contents.shifted_by(-1 * offset);
}

NoteAndBPMSelection Clipboard::paste(Fraction offset) const {
    return contents.shifted_by(offset);
}

bool Clipboard::empty() const {
    return contents.empty();
}


