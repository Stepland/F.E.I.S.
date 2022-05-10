#pragma once

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alext.h>
#include <SFML/Audio/Export.hpp>
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector3.hpp>



class SyncedSoundStreams {
public:
    enum Status {Stopped, Paused, Playing};
    struct Chunk {
        const sf::Int16* samples;     //!< Pointer to the audio samples
        std::size_t  sampleCount; //!< Number of samples pointed by Samples
    };
    template<typename T>
    struct Span {
        Span() {}
        Span(T off, T len) : offset(off), length(len) {}
        T offset; //!< The beginning offset of the time range
        T length; //!< The length of the time range
    };

    // Define the relevant Span types
    using TimeSpan = Span<sf::Time>;

    SyncedSoundStreams();
    SyncedSoundStreams(const SyncedSoundStreams& copy);
    SyncedSoundStreams& operator=(const SyncedSoundStreams& right);
    virtual ~SyncedSoundStreams();


    void setPitch(float pitch);
    void setAllVolumes(float volume);
    void setMusicVolume(float volume);
    float getPitch() const;
    float getVolume() const;

    void play();
    void pause();
    void stop();
    Status getSoundSourceStatus() const;
    Status getStatus() const;

    void setPlayingOffset(sf::Time timeOffset);
    sf::Time getPlayingOffset() const;

    sf::Time getPrecisePlayingOffset() const;

    void setLoop(bool loop);
    bool getLoop() const;

    [[nodiscard]] bool openFromFile(const std::filesystem::path& filename);

    sf::Time getMusicDuration() const;
    TimeSpan getLoopPoints() const;
    void setLoopPoints(TimeSpan timePoints);

    bool music_is_loaded() const;

    void clear_music();

protected:
    enum{NoLoop = -1}; //!< "Invalid" endSeeks value, telling us to continue uninterrupted
    sf::Int64 onLoop();
    void setProcessingInterval(sf::Time interval);

    [[nodiscard]] bool music_data_callback(Chunk& data);
    void music_seek_callback(sf::Time timeOffset);
    sf::Int64 music_loop_callback();

    [[nodiscard]] bool note_clap_data_callback(Chunk& data);
    void note_clap_seek_callback(sf::Time timeOffset);
    sf::Int64 note_clap_loop_callback();

    [[nodiscard]] bool chord_clap_data_callback(Chunk& data);
    void chord_clap_seek_callback(sf::Time timeOffset);
    sf::Int64 chord_clap_loop_callback();

    [[nodiscard]] bool beat_tick_data_callback(Chunk& data);
    void beat_tick_seek_callback(sf::Time timeOffset);
    sf::Int64 beat_tick_loop_callback();
    
    std::array<sf::Time, 2> alSecOffsetLatencySoft() const;
    sf::Time lag = sf::Time::Zero;
    LPALGETSOURCEDVSOFT alGetSourcedvSOFT;
private:

    enum {
        BufferPerSource = 3,    //!< Number of audio buffers used per-source by the streaming loop
        BufferRetries = 2   //!< Number of retries (excluding initial try) for onGetData()
    };

    std::thread m_thread; //!< Thread running the background tasks
    mutable std::recursive_mutex m_threadMutex; //!< Thread mutex
    Status m_threadStartState; //!< State the thread starts in (Playing, Paused, Stopped)
    bool m_isStreaming; //!< Streaming state (true = playing, false = stopped)
    bool m_loop; //!< Loop flag (true to loop, false to play once)
    sf::Time m_processingInterval; //!< Interval for checking and filling the internal sound buffers.

    std::array<ALuint, 4> sources;
    const unsigned int music_source_index = 0;
    const unsigned int note_clap_source_index = 1;
    const unsigned int chord_clap_source_index = 2;
    const unsigned int beat_tick_source_index = 3;

    struct StreamData {
        std::function<bool(Chunk&)> onGetData;
        std::function<void(sf::Time)> onSeek;
        std::function<sf::Int64()> onLoop;
        sf::Int32 m_format = 0;
        unsigned int m_channelCount = 0; //!< Number of channels (1 = mono, 2 = stereo, ...)
        unsigned int m_sampleRate = 0; //!< Frequency (samples / second)
        std::array<unsigned int, BufferPerSource> m_buffers; //!< Sound buffers used to store temporary audio data
        std::array<sf::Int64, BufferPerSource> m_bufferSeeks; //!< If buffer is an "end buffer", holds next seek position, else NoLoop. For play offset calculation.
        sf::Uint64 m_samplesProcessed = 0; //!< Number of samples processed since beginning of the stream
        sf::Uint64 timeToSamples(sf::Time position) const;
        sf::Time samplesToTime(sf::Uint64 samples) const;
    };

    std::array<StreamData, 4> stream_data;

    void streamData();
    [[nodiscard]] bool fillAndPushBuffer(unsigned int source_index, unsigned int bufferNum, bool immediateLoop = false);
    [[nodiscard]] bool fillQueue();
    void clearQueue();
    void launchStreamingThread(Status threadStartState);
    void awaitStreamingThread();

    void initialize_sound_stream(StreamData& stream, unsigned int channelCount, unsigned int sampleRate);
    void initialize_music();

    struct MusicData {
        sf::InputSoundFile m_file; //!< The streamed music file
        std::vector<sf::Int16> m_samples; //!< Temporary buffer of samples
        std::recursive_mutex m_mutex; //!< Mutex protecting the data
        Span<sf::Uint64> m_loopSpan = {0, 0}; //!< Loop Range Specifier
    };

    MusicData music_data;
};


