//
// Created by Symeon on 23/12/2018.
//

#include "EditorState.h"

EditorState::EditorState(Fumen &fumen) : fumen(fumen) {
    if (not this->fumen.Charts.empty()) {
        this->selectedChart = this->fumen.Charts.begin()->first;
    }
    if (this->fumen.musicPath != "") {
        this->music.emplace();
        if (!this->music.value().openFromFile(this->fumen.musicPath))
        {
            throw std::invalid_argument("Error loading audio file : "+this->fumen.musicPath);
        }
    }
}
