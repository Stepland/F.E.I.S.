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
    std::optional<Chart> selectedChart; // Ok this was a pretty terrible design choice, be EXTRA careful about this still being in sync with what's actually in the std::map of fumen

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
    bool showChartList;
    bool showNewChartDialog;
    bool showChartProperties;

    void displayPlayfield();
    void displayProperties();
    void displayStatus();
    void displayPlaybackStatus();
    void displayTimeline();
    void displayChartList();
    void displayChartProperties();

    bool playBeatTick;
    bool playNoteTick;

    std::vector<Note> getVisibleNotes();

    explicit EditorState(Fumen& fumen);
};

namespace ESHelper {
    void save(EditorState& ed);
    void open(std::optional<EditorState>& ed);
    void openFromFile(std::optional<EditorState>& ed, std::filesystem::path path);

    class NewChartDialog {

    public:

        std::optional<Chart> display(EditorState& editorState);
        void resetValues() {level = 1; resolution = 240; difficulty = ""; comboPreview = ""; showCustomDifName = false;};

    private:

        int level = 1;
        int resolution = 240;
        std::string difficulty;
        std::string comboPreview;
        bool showCustomDifName = false;

    };

    class ChartPropertiesDialog {

    public:

        void display(EditorState& editorState);
        bool shouldRefreshValues = true;

    private:

        int level;
        std::string difficulty;
        std::string comboPreview;
        std::set<std::string> difNamesInUse;
        bool showCustomDifName = false;

    };
}


#endif //FEIS_EDITORSTATE_H
