//
// Created by Syméon on 12/02/2019.
//

#ifndef FEIS_HISTORY_H
#define FEIS_HISTORY_H

#include <stack>
#include <optional>

template<typename T>
class History {
public:

    std::optional<T> get_previous() {
        if (previous_states.empty()) {
            return {};
        } else {
            T value = previous_states.back();
            previous_states.pop_back();
            next_states.push_back(value);
            return value;
        }
    }

    std::optional<T> get_next() {
        if (next_states.empty()) {
            return {};
        } else {
            T value = next_states.back();
            next_states.pop_back();
            previous_states.push_back(value);
            return value;
        }
    }

    void push(const T& elt) {
        previous_states.push_back(elt);
        if (not next_states.empty()) {
            next_states.clear();
        }
    }


private:
    std::deque<T> previous_states;
    std::deque<T> next_states;
};


#endif //FEIS_HISTORY_H
