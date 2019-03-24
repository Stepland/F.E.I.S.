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
#include "History.h"
#include "HistoryActions.h"

class ActionWithMessage;
class OpenChart;

class EditorState {
public:

    struct Chart_with_History {
        explicit Chart_with_History(Chart& c);
        Chart& ref;
        History<std::shared_ptr<ActionWithMessage>> history;
    };

    explicit EditorState(Fumen& fumen);

    Fumen fumen;
    std::optional<Chart_with_History> chart;
    Widgets::Playfield playfield;
    int snap = 1;

    std::optional<sf::Music> music;
    int musicVolume = 10; // 0 -> 10
    void setMusicVolume(int newMusicVolume);
    void musicVolumeUp() {setMusicVolume(musicVolume+1);};
    void musicVolumeDown() {setMusicVolume(musicVolume-1);};

    int musicSpeed = 10; // 1 -> 20
    void setMusicSpeed(int newMusicSpeed);
    void musicSpeedUp() {setMusicSpeed(musicSpeed+1);};
    void musicSpeedDown() {setMusicSpeed(musicSpeed-1);};

    std::optional<sf::Texture> albumCover;

    sf::Time previousPos;
    sf::Time playbackPosition;
    sf::Time chartRuntime; // sf::Time at which the chart preview stops, can be after the end of the audio

    void setPlaybackAndMusicPosition(sf::Time newPosition);

    bool playing;
    int lastTimingTicked = -1;

    float   getBeats        ()              {return getBeatsAt(playbackPosition.asSeconds());};
    float   getBeatsAt      (float seconds) {return ((seconds+fumen.offset)/60.f)* fumen.BPM;};
    float   getTicks        ()              {return getTicksAt(playbackPosition.asSeconds());};
    float   getTicksAt      (float seconds) {return getBeatsAt(seconds) * getResolution();}
    float   getSecondsAt    (int tick)      {return (60.f * tick)/(fumen.BPM * getResolution()) - fumen.offset;};
    int     getResolution   ()              {return chart ? chart->ref.getResolution() : 240;};
    int     getSnapStep     ()              {return getResolution() / snap;};

    float   ticksToSeconds  (int ticks)     {return (60.f * ticks)/(fumen.BPM * getResolution());};

    void reloadFromFumen();
    void reloadMusic();
    void reloadPlaybackPositionAndChartRuntime();
    void reloadAlbumCover();

    bool showPlayfield = true;
    bool showProperties;
    bool showStatus;
    bool showPlaybackStatus = true;
    bool showTimeline = true;
    bool showChartList;
    bool showNewChartDialog;
    bool showChartProperties;
    bool showHistory;

    void displayPlayfield(Marker& marker, MarkerEndingState markerEndingState);
    void displayProperties();
    void displayStatus();
    void displayPlaybackStatus();
    void displayTimeline();
    void displayChartList();

    void updateVisibleNotes();
    std::set<Note> visibleNotes;

    void toggleNoteAtCurrentTime(int pos);
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
        std::string difficulty_name;
        std::string comboPreview;
        std::set<std::string> difNamesInUse;
        bool showCustomDifName = false;

    };
}


#endif //FEIS_EDITORSTATE_H
