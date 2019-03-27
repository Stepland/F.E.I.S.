//
// Created by Syméon on 27/03/2019.
//

#ifndef FEIS_TIMESELECTION_H
#define FEIS_TIMESELECTION_H

#include <variant>

struct TimeSelection {

    explicit TimeSelection(unsigned int start = 0, unsigned int duration = 0) : start(start), duration(duration) {};

    unsigned int start;
    unsigned int duration;

};

typedef std::variant<std::monostate,unsigned int,TimeSelection> SelectionState;


#endif //FEIS_TIMESELECTION_H
