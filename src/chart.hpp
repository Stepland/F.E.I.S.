#ifndef FEIS_CHART_H
#define FEIS_CHART_H

#include <iostream>
#include <set>
#include <vector>

#include "note.hpp"

/*
 * Holds the notes, the difficulty name and the level
 */
class Chart {
public:
    Chart(const std::string& dif = "Edit", int level = 1, int resolution = 240);

    int getResolution() const;
    void setResolution(int resolution);

    std::string dif_name;
    int level;
    std::set<Note> Notes;

    std::set<Note> getNotesBetween(int start_timing, int end_timing) const;
    std::set<Note> getVisibleNotesBetween(int start_timing, int end_timing) const;

    bool is_colliding(const Note& note, int ticks_threshold);

    bool operator==(const Chart& rhs) const;

    bool operator!=(const Chart& rhs) const;

private:
    int resolution;
};

#endif  // FEIS_CHART_H
