#ifndef FEIS_EDITORSTATE_H
#define FEIS_EDITORSTATE_H

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <optional>

#include "chart_with_history.hpp"
#include "fumen.hpp"
#include "history.hpp"
#include "history_actions.hpp"
#include "marker.hpp"
#include "notes_clipboard.hpp"
#include "time_selection.hpp"
#include "widgets/linear_view.hpp"
#include "widgets/playfield.hpp"

class ActionWithMessage;
class OpenChart;

enum saveChangesResponses {
    saveChangesYes,
    saveChangesNo,
    saveChangesCancel,
    saveChangesDidNotDisplayDialog
};

/*
 * The god class, holds everything there is to know about the currently open
 * .memon file
 */
class EditorState {
public:
    EditorState(Fumen& fumen, std::filesystem::path assets);

    std::optional<Chart_with_History> chart;

    Fumen fumen;

    Playfield playfield;
    LinearView linearView;

    // the snap but divided by 4 because you can't set a snap to anything lower
    // than 4ths
    int snap = 1;

    std::optional<sf::Music> music;
    int musicVolume = 10;  // 0 -> 10
    void setMusicVolume(int newMusicVolume);
    void musicVolumeUp() { setMusicVolume(musicVolume + 1); };
    void musicVolumeDown() { setMusicVolume(musicVolume - 1); };

    int musicSpeed = 10;  // 1 -> 20
    void setMusicSpeed(int newMusicSpeed);
    void musicSpeedUp() { setMusicSpeed(musicSpeed + 1); };
    void musicSpeedDown() { setMusicSpeed(musicSpeed - 1); };

    std::optional<sf::Texture> albumCover;

    bool playing;

    sf::Time previousPos;
    sf::Time playbackPosition;

private:
    sf::Time previewEnd;  // sf::Time (in the audio file "coordinates") at which the chart preview stops, can be
                          // after the end of the actual audio file

public:
    const sf::Time& getPreviewEnd();

public:
    void setPlaybackAndMusicPosition(sf::Time newPosition);

    float getBeats() { return getBeatsAt(playbackPosition.asSeconds()); };
    float getBeatsAt(float seconds) {
        return ((seconds + fumen.offset) / 60.f) * fumen.BPM;
    };
    float getCurrentTick() { return getTicksAt(playbackPosition.asSeconds()); };
    float getTicksAt(float seconds) {
        return getBeatsAt(seconds) * getResolution();
    }
    float getSecondsAt(int tick) {
        return (60.f * tick) / (fumen.BPM * getResolution()) - fumen.offset;
    };
    int getResolution() { return chart ? chart->ref.getResolution() : 240; };
    int getSnapStep() { return getResolution() / snap; };

    float ticksToSeconds(int ticks) {
        return (60.f * ticks) / (fumen.BPM * getResolution());
    };

    float getChartRuntime() {
        return getPreviewEnd().asSeconds() + fumen.offset;
    };

    void reloadFromFumen(std::filesystem::path assets);
    void reloadMusic();
    void reloadAlbumCover();
    void reloadPreviewEnd();

    bool showPlayfield = true;
    bool showProperties;
    bool showStatus;
    bool showPlaybackStatus = true;
    bool showTimeline = true;
    bool showChartList;
    bool showNewChartDialog;
    bool showChartProperties;
    bool showHistory;
    bool showSoundSettings;
    bool showLinearView;

    void displayPlayfield(Marker& marker, MarkerEndingState markerEndingState);
    void displayProperties();
    void displayStatus();
    void displayPlaybackStatus();
    void displayTimeline();
    void displayChartList(std::filesystem::path assets);
    void displayLinearView();

    saveChangesResponses alertSaveChanges();
    bool saveChangesOrCancel();

    void updateVisibleNotes();
    std::set<Note> visibleNotes;

    void toggleNoteAtCurrentTime(int pos);
};

namespace ESHelper {
    void save(EditorState& ed);
    void open(std::optional<EditorState>& ed, std::filesystem::path assets, std::filesystem::path settings);
    void openFromFile(
        std::optional<EditorState>& ed,
        std::filesystem::path file,
        std::filesystem::path assets,
        std::filesystem::path settings
    );

    bool saveOrCancel(std::optional<EditorState>& ed);

    class NewChartDialog {
    public:
        std::optional<Chart> display(EditorState& editorState);
        void resetValues() {
            level = 1;
            resolution = 240;
            difficulty = "";
            comboPreview = "";
            showCustomDifName = false;
        };

    private:
        int level = 1;
        int resolution = 240;
        std::string difficulty;
        std::string comboPreview;
        bool showCustomDifName = false;
    };

    class ChartPropertiesDialog {
    public:
        void display(EditorState& editorState, std::filesystem::path assets);
        bool shouldRefreshValues = true;

    private:
        int level;
        std::string difficulty_name;
        std::string comboPreview;
        std::set<std::string> difNamesInUse;
        bool showCustomDifName = false;
    };
}

#endif  // FEIS_EDITORSTATE_H
