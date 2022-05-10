#pragma once

#include <string>

#include <SFML/System/Vector3.hpp>


class AudioDevice {
public:
    AudioDevice();
    ~AudioDevice();

    static bool isExtensionSupported(const std::string& extension);
    static int getFormatFromChannelCount(unsigned int channelCount);
    static void setGlobalVolume(float volume);
    static float getGlobalVolume();
    static void setPosition(const sf::Vector3f& position);
    static sf::Vector3f getPosition();
    static void setDirection(const sf::Vector3f& direction);
    static sf::Vector3f getDirection();
    static void setUpVector(const sf::Vector3f& upVector);
    static sf::Vector3f getUpVector();
};
