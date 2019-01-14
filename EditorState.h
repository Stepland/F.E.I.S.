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
#include "Widgets.h"

class EditorState {

public:
    Fumen fumen;
    Widgets::Playfield playfield;
    MarkerEndingState markerEndingState;
    std::optional<sf::Music> music;
    std::optional<sf::Texture> jacket;
    std::optional<Chart> selectedChart;

    sf::Time playbackPosition;
    sf::Time chartRuntime; // Timing at which the playback stops
    bool playing;
    float getBeats() {return getBeatsAt(playbackPosition.asSeconds());};
    float getBeatsAt(float seconds) {return ((seconds+fumen.offset)/60.f)* fumen.BPM;};
    float getTicks() {return getTicksAt(playbackPosition.asSeconds());};
    float getTicksAt(float seconds) {
        if (selectedChart) {
            return getBeatsAt(seconds)*selectedChart->getResolution();
        } else {
            return getBeatsAt(seconds)*240.f;
        }
    }
    float getSecondsAt(int tick) {
        if (selectedChart) {
            return (60.f * tick)/(fumen.BPM * selectedChart->getResolution()) - fumen.offset;
        } else {
            return (60.f * tick)/(fumen.BPM * 240.f) - fumen.offset;
        }
    }

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

    bool playBeatTick;
    bool playNoteTick;

    std::vector<Note> getVisibleNotes();

    explicit EditorState(Fumen& fumen);
};

namespace ESHelper {
    void save(EditorState& ed);
    void open(std::optional<EditorState>& ed);
    void openFromFile(std::optional<EditorState>& ed, std::filesystem::path path);
}


#endif //FEIS_EDITORSTATE_H
