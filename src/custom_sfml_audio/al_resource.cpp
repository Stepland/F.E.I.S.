#include "al_resource.hpp"

#include <memory>
#include <mutex>

#include "audio_device.hpp"

namespace {
    // OpenAL resources counter and its mutex
    unsigned int count = 0;
    std::recursive_mutex mutex;

    // The audio device is instantiated on demand rather than at global startup,
    // which solves a lot of weird crashes and errors.
    // It is destroyed when it is no longer needed.
    std::unique_ptr<AudioDevice> globalDevice;
}

AlResource::AlResource() {
    // Protect from concurrent access
    std::scoped_lock lock(mutex);

    // If this is the very first resource, trigger the global device initialization
    if (count == 0)
        globalDevice = std::make_unique<AudioDevice>();

    // Increment the resources counter
    ++count;
}

AlResource::~AlResource() {
    // Protect from concurrent access
    std::scoped_lock lock(mutex);

    // Decrement the resources counter
    --count;

    // If there's no more resource alive, we can destroy the device
    if (count == 0)
        globalDevice.reset();
}
