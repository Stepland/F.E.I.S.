#include "audio_device.hpp"

#include <iostream>
#include <optional>
#include <ostream>

#include <SFML/Audio/Listener.hpp>

#include "al_check.hpp"

#if defined(__APPLE__)
    #if defined(__clang__)
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__GNUC__)
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif

namespace {
    ALCdevice*  audioDevice  = nullptr;
    ALCcontext* audioContext = nullptr;

    float        listenerVolume = 100.f;
    sf::Vector3f listenerPosition (0.f, 0.f, 0.f);
    sf::Vector3f listenerDirection(0.f, 0.f, -1.f);
    sf::Vector3f listenerUpVector (0.f, 1.f, 0.f);
}

AudioDevice::AudioDevice() {
    // Create the device
    audioDevice = alcOpenDevice(nullptr);
    if (audioDevice) {
        // Create the context
        audioContext = alcCreateContext(audioDevice, nullptr);
        if (audioContext) {
            // Set the context as the current one (we'll only need one)
            alcMakeContextCurrent(audioContext);

            // Apply the listener properties the user might have set
            float orientation[] = {
                listenerDirection.x,
                listenerDirection.y,
                listenerDirection.z,
                listenerUpVector.x,
                listenerUpVector.y,
                listenerUpVector.z
            };
            alCheck(alListenerf(AL_GAIN, listenerVolume * 0.01f));
            alCheck(alListener3f(AL_POSITION, listenerPosition.x, listenerPosition.y, listenerPosition.z));
            alCheck(alListenerfv(AL_ORIENTATION, orientation));
        } else {
            std::cerr << "Failed to create the audio context" << std::endl;
        }
    } else {
        std::cerr << "Failed to open the audio device" << std::endl;
    }
}

AudioDevice::~AudioDevice() {
    // Destroy the context
    alcMakeContextCurrent(nullptr);
    if (audioContext) {
        alcDestroyContext(audioContext);
    }

    // Destroy the device
    if (audioDevice) {
        alcCloseDevice(audioDevice);
    }
}

bool AudioDevice::isExtensionSupported(const std::string& extension) {
    // Create a temporary audio device in case none exists yet.
    // This device will not be used in this function and merely
    // makes sure there is a valid OpenAL device for extension
    // queries if none has been created yet.
    std::optional<AudioDevice> device;
    if (!audioDevice)
        device.emplace();

    if ((extension.length() > 2) && (extension.substr(0, 3) == "ALC"))
        return alcIsExtensionPresent(audioDevice, extension.c_str()) != AL_FALSE;
    else
        return alIsExtensionPresent(extension.c_str()) != AL_FALSE;
}

int AudioDevice::getFormatFromChannelCount(unsigned int channelCount) {
    // Create a temporary audio device in case none exists yet.
    // This device will not be used in this function and merely
    // makes sure there is a valid OpenAL device for format
    // queries if none has been created yet.
    std::optional<AudioDevice> device;
    if (!audioDevice) {
        device.emplace();
    }

    // Find the good format according to the number of channels
    int format = 0;
    switch (channelCount) {
        case 1:
            format = AL_FORMAT_MONO16;
            break;
        case 2:
            format = AL_FORMAT_STEREO16;
            break;
        case 4:
            format = alGetEnumValue("AL_FORMAT_QUAD16");
            break;
        case 6:
            format = alGetEnumValue("AL_FORMAT_51CHN16");
            break;
        case 7:
            format = alGetEnumValue("AL_FORMAT_61CHN16");
            break;
        case 8:
            format = alGetEnumValue("AL_FORMAT_71CHN16");
            break;
        default:
            format = 0;
            break;
    }

    // Fixes a bug on OS X
    if (format == -1) {
        format = 0;
    }

    return format;
}

void AudioDevice::setGlobalVolume(float volume) {
    if (audioContext) {
        alCheck(alListenerf(AL_GAIN, volume * 0.01f));
    }

    listenerVolume = volume;
}

float AudioDevice::getGlobalVolume() {
    return listenerVolume;
}

void AudioDevice::setPosition(const sf::Vector3f& position) {
    if (audioContext) {
        alCheck(alListener3f(AL_POSITION, position.x, position.y, position.z));
    }

    listenerPosition = position;
}

sf::Vector3f AudioDevice::getPosition() {
    return listenerPosition;
}

void AudioDevice::setDirection(const sf::Vector3f& direction) {
    if (audioContext)
    {
        float orientation[] = {direction.x, direction.y, direction.z, listenerUpVector.x, listenerUpVector.y, listenerUpVector.z};
        alCheck(alListenerfv(AL_ORIENTATION, orientation));
    }

    listenerDirection = direction;
}

sf::Vector3f AudioDevice::getDirection() {
    return listenerDirection;
}

void AudioDevice::setUpVector(const sf::Vector3f& upVector) {
    if (audioContext) {
        float orientation[] = {listenerDirection.x, listenerDirection.y, listenerDirection.z, upVector.x, upVector.y, upVector.z};
        alCheck(alListenerfv(AL_ORIENTATION, orientation));
    }

    listenerUpVector = upVector;
}

sf::Vector3f AudioDevice::getUpVector() {
    return listenerUpVector;
}
