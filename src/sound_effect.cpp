#include <imgui/imgui.h>
#include "sound_effect.hpp"
#include "toolbox.hpp"

SoundEffect::SoundEffect(std::string filename): buffer(), volume(10), shouldPlay(false) {

    auto soundPath = std::filesystem::path("assets/sounds") / filename;

    if (!buffer.loadFromFile(soundPath.string())) {
        std::cerr << "Unable to load sound : " << filename;
        throw std::runtime_error("Unable to load sound : " + filename);
    }

    sound = sf::Sound(buffer);
}

void SoundEffect::play() {
    sound.play();
}

void SoundEffect::setVolume(int newVolume) {
    volume = std::clamp(newVolume,0,10);
    Toolbox::updateVolume(sound,volume);
}

int SoundEffect::getVolume() const {
    return volume;
}

void SoundEffect::displayControls() {
    ImGui::PushID(&shouldPlay);
    ImGui::Checkbox("Toggle",&shouldPlay); ImGui::SameLine();
    ImGui::PopID();

    ImGui::PushID(&volume);
    if (ImGui::SliderInt("Volume",&volume,0,10)) {
        setVolume(volume);
    }
    ImGui::PopID();
}
