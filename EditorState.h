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
    std::optional<Chart> selectedChart; // Ok this was a pretty terrible design choice, be EXTRA careful about this still being in sync with what's actually in the std::map of fumen
    Widgets::Playfield playfield;
    int snap = 1;

    std::optional<sf::Music> music;
    int musicVolume = 10; // 0 -> 10
    void updateMusicVolume();

    std::optional<sf::Texture> jacket;

    sf::Time playbackPosition;
    sf::Time chartRuntime; // sf::Time at which the chart preview stops, can be after the end of the audio

    void setPlaybackAndMusicPosition(sf::Time newPosition);

    bool playing;

    float getBeats() {return getBeatsAt(playbackPosition.asSeconds());};
    float getBeatsAt(float seconds) {return ((seconds+fumen.offset)/60.f)* fumen.BPM;};
    float getTicks() {return getTicksAt(playbackPosition.asSeconds());};
    float getTicksAt(float seconds) {return getBeatsAt(seconds) * getResolution();}
    float getSecondsAt(int tick) {return (60.f * tick)/(fumen.BPM * getResolution()) - fumen.offset;};
    int getResolution() {return selectedChart ? selectedChart->getResolution() : 240;};
    int getSnapStep() {return getResolution() / snap;};

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
