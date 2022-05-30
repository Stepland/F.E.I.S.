#pragma once

#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <SFML/Audio/Export.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <SFML/System/Time.hpp>

#include "open_sound_stream.hpp"


// Number of audio buffers used by the streaming loop
constexpr auto BufferCount = 3;
// Number of retries (excluding initial try) for onGetData()
constexpr auto BufferRetries = 2;
// "Invalid" endSeeks value, telling us to continue uninterrupted
constexpr auto NoLoop = -1;

struct Buffers {
    // Sound buffers used to store temporary audio data
    std::array<unsigned int, BufferCount> m_buffers = {0, 0, 0};
    // Number of channels (1 = mono, 2 = stereo, ...)
    unsigned int m_channelCount = 0;
    // Frequency (samples / second)
    unsigned int m_sampleRate = 0;
    // Format of the internal sound buffers
    sf::Int32 m_format = 0;
    // Number of samples processed since beginning of the stream
    sf::Uint64 m_samplesProcessed = 0;
    // If buffer is an "end buffer", holds next seek position, else NoLoop. For play offset calculation.
    std::array<sf::Int64, BufferCount> m_bufferSeeks = {0, 0, 0};
};

struct InternalStream {
    std::shared_ptr<OpenSoundStream> stream;
    Buffers buffers;

    void clear_queue();
};

class SyncedSoundStreams {
public:
    ~SyncedSoundStreams();

    void add_stream(const std::string& name, std::shared_ptr<OpenSoundStream> s);
    void remove_stream(const std::string& name);

    void play();
    void pause();
    void stop();

    sf::SoundSource::Status getStatus() const;

    void setPlayingOffset(sf::Time timeOffset);
    sf::Time getPlayingOffset() const;

    void setLoop(bool loop);
    bool getLoop() const;

protected:
    SyncedSoundStreams();
    void setProcessingInterval(sf::Time interval);

private:
    void streamData();
    [[nodiscard]] bool fillAndPushBuffer(InternalStream& stream, unsigned int bufferNum, bool immediateLoop = false);
    [[nodiscard]] bool fillQueues();
    void clearQueues();
    void launchStreamingThread(sf::SoundSource::Status threadStartState);
    void awaitStreamingThread();

    void reload_sources();

    std::thread m_thread; // Thread running the background tasks
    mutable std::recursive_mutex m_threadMutex; // Thread mutex
    sf::SoundSource::Status m_threadStartState; // State the thread starts in (Playing, Paused, Stopped)
    bool m_isStreaming; // Streaming state (true = playing, false = stopped)
    bool m_loop; // Loop flag (true to loop, false to play once)
    sf::Time m_processingInterval; // Interval for checking and filling the internal sound buffers.
    std::map<std::string, InternalStream> streams;
    std::vector<ALuint> sources;
};