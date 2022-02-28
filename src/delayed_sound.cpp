#include "delayed_sound.hpp"

#include <algorithm>
#include <cstddef>

#include <SFML/Config.hpp>

DelayedSound::DelayedSound(const std::filesystem::path& sound_file) :
    voices(16),
    audio()
{
    sf::SoundBuffer raw_audio;
    if (not raw_audio.loadFromFile(sound_file.c_str())) {
        throw std::runtime_error("Couldn't load file "+sound_file.string());
    }

    // prefix with one second of silence
    std::vector<sf::Int16> samples(raw_audio.getSampleRate(), 0);
    auto first_sample = raw_audio.getSamples();
    std::vector<sf::Int16> sound_samples(first_sample, first_sample + raw_audio.getSampleCount());
    samples.insert(samples.end(), sound_samples.begin(), sound_samples.end());
    bool success = audio.loadFromSamples(samples.data(), samples.size(), raw_audio.getChannelCount(), raw_audio.getSampleRate());
    if (not success) {
        throw std::runtime_error(
            "Error while constructing DelayedSound"
            + sound_file.string()
            + " when loading samples after inserting the padding silence"
        );
    }

    for (auto& voice : voices) {
        voice.setBuffer(audio);
    }
}

void DelayedSound::playIn(const sf::Time& time) {
    if (time < sf::Time::Zero or time > sf::seconds(1)) {
        return;
    }

    auto is_usable = [](const sf::Sound& s){ return s.getStatus() != sf::Sound::Playing; };
    auto voice_it = std::find_if(voices.begin(), voices.end(), is_usable);
    if (voice_it != voices.end()) {
        voice_it->play();
        voice_it->setPlayingOffset(sf::seconds(1) - time);
    }

    // Give up playing the sound if every voice is taken
}

void DelayedSound::setVolume(float volume) {
    for (auto& voice: voices) {
        voice.setVolume(volume);
    }
}