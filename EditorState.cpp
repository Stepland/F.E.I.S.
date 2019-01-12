//
// Created by Symeon on 23/12/2018.
//

#include <filesystem>
#include "EditorState.h"

EditorState::EditorState(Fumen &fumen) : fumen(fumen) {
    if (not this->fumen.Charts.empty()) {
        this->selectedChart = this->fumen.Charts.begin()->first;
    }
    this->music.emplace();
    try {
        if (!this->music.value().openFromFile(
                (fumen.path.parent_path() / std::filesystem::path(this->fumen.musicPath)).string())) {
            this->music.reset();
        }
    } catch (const std::exception& e) {
        this->music.reset();
    }
}

void EditorState::reloadMusic() {
    this->music.emplace();
    try {
        if (!this->music.value().openFromFile(
                (fumen.path.parent_path() / std::filesystem::path(this->fumen.musicPath)).string())) {
            this->music.reset();
        }
    } catch (const std::exception& e) {
        this->music.reset();
    }
}
