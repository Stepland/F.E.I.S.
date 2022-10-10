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

#include "al_resource.hpp"
#include "precise_sound_stream.hpp"
#include "src/history_item.hpp"


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

struct NewStream {
    std::shared_ptr<PreciseSoundStream> stream;
    bool bypasses_openal_pitch;
};

struct InternalStream {
    std::shared_ptr<PreciseSoundStream> stream;
    Buffers buffers;
    bool bypasses_openal_pitch;

    void clear_queue();
};

class SyncedSoundStreams : public AlResource {
public:
    SyncedSoundStreams();
    ~SyncedSoundStreams();

    void update_streams(
        const std::map<std::string, NewStream>& to_add,
        const std::initializer_list<std::string>& to_remove = {},
        std::optional<float> new_pitch = {}
    );
    void add_stream(const std::string& name, NewStream s);
    void remove_stream(const std::string& name);
    bool contains_stream(const std::string& name) const;
    bool empty() const;

    void play();
    void pause();
    void stop();

    sf::SoundSource::Status getStatus() const;

    void setPlayingOffset(const sf::Time timeOffset);
    sf::Time getPlayingOffset() const;
    sf::Time getPrecisePlayingOffset() const;

    void setPitch(float pitch);

    void setLoop(bool loop);
    bool getLoop() const;

    void display_debug() const;

protected:
    void setProcessingInterval(sf::Time interval);

private:
    void change_streams(
        std::function<void()> callback,
        std::optional<float> new_pitch = {}
    );
    void add_stream_internal(const std::string& name, NewStream s);
    void remove_stream_internal(const std::string& name);
    void streamData();
    [[nodiscard]] bool fillAndPushBuffer(InternalStream& stream, unsigned int bufferNum, bool immediateLoop = false);
    [[nodiscard]] bool fillQueues();
    void clearQueues();
    void launchStreamingThread(sf::SoundSource::Status threadStartState);
    void awaitStreamingThread();

    void unsafe_update_streams();
    void reload_sources();

    sf::Time getPlayingOffset(const InternalStream& s) const;
    sf::Time getPrecisePlayingOffset(const InternalStream& s) const;

    float pitch = 1.f;
    std::thread m_thread; // Thread running the background tasks
    mutable std::recursive_mutex m_threadMutex; // Thread mutex
    sf::SoundSource::Status m_threadStartState; // State the thread starts in (Playing, Paused, Stopped)
    bool m_isStreaming; // Streaming state (true = playing, false = stopped)
    bool m_loop; // Loop flag (true to loop, false to play once)
    sf::Time m_processingInterval; // Interval for checking and filling the internal sound buffers.
    std::map<std::string, InternalStream> streams;
    std::vector<ALuint> sources;
};