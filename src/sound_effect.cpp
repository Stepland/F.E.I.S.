#include "sound_effect.hpp"

#include <imgui/imgui.h>

#include "toolbox.hpp"
#include "utf8_strings.hpp"

SoundEffect::SoundEffect(std::filesystem::path path) :
    shouldPlay(false),
    buffer(),
    volume(10) {
    if (!buffer.loadFromFile(to_sfml_string(path))) {
        throw std::runtime_error("Unable to load sound : " + path.string());
    }

    sound = sf::Sound(buffer);
}

void SoundEffect::play() {
    sound.play();
}

void SoundEffect::setVolume(int newVolume) {
    volume = std::clamp(newVolume, 0, 10);
    sound.setVolume(Toolbox::convertVolumeToNormalizedDB(volume));
}

int SoundEffect::getVolume() const {
    return volume;
}

void SoundEffect::displayControls() {
    ImGui::PushID(&shouldPlay);
    ImGui::Checkbox("Toggle", &shouldPlay);
    ImGui::SameLine();
    ImGui::PopID();

    ImGui::PushID(&volume);
    if (ImGui::SliderInt("Volume", &volume, 0, 10)) {
        setVolume(volume);
    }
    ImGui::PopID();
}
