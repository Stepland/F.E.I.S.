//
// Created by Symeon on 23/12/2018.
//

#ifndef FEIS_EDITORSTATE_H
#define FEIS_EDITORSTATE_H

#include <optional>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "Fumen.h"
#include "Marker.h"

class EditorState {

public:
    Fumen fumen;
    Marker marker;
    std::optional<sf::Music> music;
    std::optional<sf::Texture> jacket;
    std::optional<Chart> selectedChart;

    sf::Time playbackPosition;
    sf::Time chartRuntime; // Timing at which the playback stops
    bool playing;
    float getBeats() {return ((playbackPosition.asSeconds()+fumen.offset)/60.f)* fumen.BPM;};

    void reloadFromFumen();
    void reloadMusic();
    void reloadPlaybackPositionAndChartRuntime();
    void reloadJacket();

    bool showPlayfield = true;
    bool showProperties;
    bool showStatus;
    bool showPlaybackStatus = true;
    bool showTimeline = true;

    void displayPlayfield();
    void displayProperties();
    void displayStatus();
    void displayPlaybackStatus();
    void displayTimeline();

    explicit EditorState(Fumen& fumen);
};

namespace ESHelper {
    void save(EditorState& ed);
    void open(std::optional<EditorState>& ed);
    void openFromFile(std::optional<EditorState>& ed, std::filesystem::path path);
}


#endif //FEIS_EDITORSTATE_H
