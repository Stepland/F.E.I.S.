//
// Created by symeon on 06/04/19.
//

#ifndef FEIS_LINEARVIEW_H
#define FEIS_LINEARVIEW_H


#include <SFML/Graphics.hpp>
#include <cmath>
#include "../TimeSelection.h"
#include "../Toolbox.h"
#include "../ChartWithHistory.h"


class LinearView {

public:

    LinearView();

    sf::RenderTexture view;
    sf::Font beat_number_font;
    sf::RectangleShape cursor;
    sf::RectangleShape selection;
    sf::RectangleShape note_rect;
    sf::RectangleShape long_note_rect;
    sf::RectangleShape long_note_collision_zone;
    sf::RectangleShape note_selected;
    sf::RectangleShape note_collision_zone;

    void resize(unsigned int width, unsigned int height);

    void update(
        const std::optional<Chart_with_History>& chart,
        const sf::Time& playbackPosition,
        const float& ticksAtPlaybackPosition,
        const float& BPM,
        const int& resolution,
        const ImVec2& size
    );

void setZoom(int zoom);
void zoom_in() {setZoom(zoom+1);};
void zoom_out() {setZoom(zoom-1);};
float timeFactor() {return std::pow(1.25f, static_cast<float>(zoom));};

bool shouldDisplaySettings;

void displaySettings();

private:

int zoom = 0;
    std::__cxx11::string font_path = "assets/fonts/NotoSans-Medium.ttf";

};

#endif //FEIS_LINEARVIEW_H
