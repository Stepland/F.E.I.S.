//
// Created by Symeon on 23/12/2018.
//

#ifndef FEIS_EDITORSTATE_H
#define FEIS_EDITORSTATE_H

#include <optional>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "Fumen.h"

class EditorState {

public:
    Fumen fumen;
    std::optional<sf::Music> music;
    std::optional<sf::Texture> jacket;
    std::optional<std::string> selectedChart;

    void reloadMusic();
    void reloadJacket();

    bool showProperties;
    bool showStatus;
    bool showPlaybackStatus = true;

    void displayProperties();
    void displayStatus();
    void displayPlaybackStatus();


    explicit EditorState(Fumen& fumen);
};


#endif //FEIS_EDITORSTATE_H
