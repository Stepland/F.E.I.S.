//
// Created by Symeon on 23/12/2018.
//

#ifndef FEIS_EDITORSTATE_H
#define FEIS_EDITORSTATE_H

#include <optional>
#include <SFML/Audio.hpp>
#include "Fumen.h"

class EditorState {

public:
    Fumen fumen;
    std::optional<sf::Music> music;
    std::optional<std::string> selectedChart;
    bool showProperties;
    bool showStatus;

    void reloadMusic();

    explicit EditorState(Fumen& fumen);
};


#endif //FEIS_EDITORSTATE_H
