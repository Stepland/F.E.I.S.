#include "synced_sound_streams.hpp"

#include <SFML/Audio/InputSoundFile.hpp>
#include <algorithm>
#include <bits/ranges_algo.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <cassert>

#include <SFML/System/Time.hpp>
#include <SFML/System/Sleep.hpp>

#include "audio_device.hpp"
#include "al_check.hpp"

#ifdef _MSC_VER
    #pragma warning(disable: 4355) // 'this' used in base member initializer list
#endif

#if defined(__APPLE__)
    #if defined(__clang__)
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__GNUC__)
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif

namespace {
    // OpenAL resources counter and its mutex
    unsigned int count = 0;
    std::recursive_mutex mutex;

    // The audio device is instantiated on demand rather than at global startup,
    // which solves a lot of weird crashes and errors.
    // It is destroyed when it is no longer needed.
    std::unique_ptr<AudioDevice> globalDevice;
}

SyncedSoundStreams::SyncedSoundStreams() :
    m_thread(),
    m_threadMutex(),
    m_threadStartState(Stopped),
    m_isStreaming(false),
    m_loop(false),
    m_processingInterval(sf::milliseconds(10))
{
    {
        std::scoped_lock lock(mutex); // Protect from concurrent access
        // If this is the very first resource, trigger the global device initialization
        if (count == 0) {
            globalDevice = std::make_unique<AudioDevice>();
        }
        ++count; // Increment the resources counter
    }
    alCheck(alGenSources(4, sources.data()));
    for (auto source: sources) {
        alCheck(alSourcei(source, AL_BUFFER, 0));
    }
    if (not alIsExtensionPresent("AL_SOFT_source_latency")) {
        throw std::runtime_error("Error: AL_SOFT_source_latency not supported");
    }
    alGetSourcedvSOFT = reinterpret_cast<LPALGETSOURCEDVSOFT>(alGetProcAddress("alGetSourcedvSOFT"));

    stream_data[music_source_index].onGetData = [this](Chunk& data){return this->music_data_callback(data);};
    stream_data[music_source_index].onSeek = [this](sf::Time t){return this->music_seek_callback(t);};
    stream_data[music_source_index].onLoop = [this](){return this->music_loop_callback();};

    stream_data[note_clap_source_index].onGetData = [this](Chunk& data){return this->note_clap_data_callback(data);};
    stream_data[note_clap_source_index].onSeek = [this](sf::Time t){return this->note_clap_seek_callback(t);};
    stream_data[note_clap_source_index].onLoop = [this](){return this->note_clap_loop_callback();};

    stream_data[chord_clap_source_index].onGetData = [this](Chunk& data){return this->chord_clap_data_callback(data);};
    stream_data[chord_clap_source_index].onSeek = [this](sf::Time t){return this->chord_clap_seek_callback(t);};
    stream_data[chord_clap_source_index].onLoop = [this](){return this->chord_clap_loop_callback();};

    stream_data[beat_tick_source_index].onGetData = [this](Chunk& data){return this->beat_tick_data_callback(data);};
    stream_data[beat_tick_source_index].onSeek = [this](sf::Time t){return this->beat_tick_seek_callback(t);};
    stream_data[beat_tick_source_index].onLoop = [this](){return this->beat_tick_loop_callback();};
}

SyncedSoundStreams::SyncedSoundStreams(const SyncedSoundStreams& copy) {
    alCheck(alGenSources(4, sources.data()));
    for (auto source: sources) {
        alCheck(alSourcei(source, AL_BUFFER, 0));
    }

    setPitch(copy.getPitch());
    setAllVolumes(copy.getVolume());
}

SyncedSoundStreams::~SyncedSoundStreams() {
    // Stop the sound if it was playing
    // We must stop before destroying the file
    stop();
    // Wait for the thread to join
    awaitStreamingThread();
    for (auto source: sources) {
        alCheck(alSourcei(source, AL_BUFFER, 0));
    }
    alCheck(alDeleteSources(4, sources.data()));
    {
        std::scoped_lock lock(mutex); // Protect from concurrent access
        --count; // Decrement the resources counter
        // If there's no more resource alive, we can destroy the device
        if (count == 0) {
            globalDevice.reset();
        }
    }
}

void SyncedSoundStreams::setPitch(float pitch) {
    for (auto source: sources) {
        alCheck(alSourcef(source, AL_PITCH, pitch));
    }
}

void SyncedSoundStreams::setAllVolumes(float volume) {
    for (auto source: sources) {
        alCheck(alSourcef(source, AL_GAIN, volume * 0.01f));
    }
}

void SyncedSoundStreams::setMusicVolume(float volume) {
    const auto source = sources[music_source_index];
    alCheck(alSourcef(source, AL_GAIN, volume * 0.01f));
}

float SyncedSoundStreams::getPitch() const {
    ALfloat pitch;
    alCheck(alGetSourcef(sources[music_source_index], AL_PITCH, &pitch));
    return pitch;
}

float SyncedSoundStreams::getVolume() const {
    ALfloat gain;
    alCheck(alGetSourcef(sources[music_source_index], AL_GAIN, &gain));
    return gain * 100.f;
}


SyncedSoundStreams& SyncedSoundStreams::operator=(const SyncedSoundStreams& right) {
    // Leave m_source untouched -- it's not necessary to destroy and
    // recreate the OpenAL sound source, hence no copy-and-swap idiom

    // Assign the sound attributes
    setPitch(right.getPitch());
    setAllVolumes(right.getVolume());

    return *this;
}

SyncedSoundStreams::Status SyncedSoundStreams::getSoundSourceStatus() const {
    ALint status;
    alCheck(alGetSourcei(sources[music_source_index], AL_SOURCE_STATE, &status));

    switch (status)
    {
        case AL_INITIAL:
        case AL_STOPPED: return Stopped;
        case AL_PAUSED:  return Paused;
        case AL_PLAYING: return Playing;
    }

    return Stopped;
}

void SyncedSoundStreams::initialize_sound_stream(StreamData& stream, unsigned int channelCount, unsigned int sampleRate) {
    stream.m_channelCount = channelCount;
    stream.m_sampleRate = sampleRate;
    stream.m_samplesProcessed = 0;

    {
        std::scoped_lock lock(m_threadMutex);
        m_isStreaming = false;
    }

    // Deduce the format from the number of channels
    stream.m_format = AudioDevice::getFormatFromChannelCount(channelCount);

    // Check if the format is valid
    if (stream.m_format == 0) {
        stream.m_channelCount = 0;
        stream.m_sampleRate   = 0;
        std::cerr << "Unsupported number of channels (" << stream.m_channelCount << ")" << std::endl;
    }
}

void SyncedSoundStreams::play() {
    // Check if the sound parameters have been set
    for (const auto& stream : stream_data) {
        if (stream.m_format == 0) {
            std::cerr << "Failed to play audio stream: sound parameters have not been initialized (call initialize() first)" << std::endl;
            return;
        }
    }

    bool isStreaming = false;
    Status threadStartState = Stopped;

    {
        std::scoped_lock lock(m_threadMutex);

        isStreaming = m_isStreaming;
        threadStartState = m_threadStartState;
    }


    if (isStreaming && (threadStartState == Paused)) {
        // If the sound is paused, resume it
        std::scoped_lock lock(m_threadMutex);
        m_threadStartState = Playing;
        alCheck(alSourcePlayv(4, sources.data()));
        return;
    } else if (isStreaming && (threadStartState == Playing)) {
        // If the sound is playing, stop it and continue as if it was stopped
        stop();
    } else if (!isStreaming && m_thread.joinable()) {
        // If the streaming thread reached its end, let it join so it can be restarted.
        // Also reset the playing offset at the beginning.
        stop();
    }

    // Start updating the stream in a separate thread to avoid blocking the application
    launchStreamingThread(Playing);

    lag = this->alSecOffsetLatencySoft()[1];
}

void SyncedSoundStreams::pause() {
    // Handle pause() being called before the thread has started
    {
        std::scoped_lock lock(m_threadMutex);

        if (!m_isStreaming)
            return;

        m_threadStartState = Paused;
    }

    alCheck(alSourcePausev(4, sources.data()));
}

void SyncedSoundStreams::stop() {
    // Wait for the thread to join
    awaitStreamingThread();

    // Move to the beginning
    music_seek_callback(sf::Time::Zero);
}

SyncedSoundStreams::Status SyncedSoundStreams::getStatus() const {
    Status status = getSoundSourceStatus();

    // To compensate for the lag between play() and alSourceplay()
    if (status == Stopped) {
        std::scoped_lock lock(m_threadMutex);

        if (m_isStreaming) {
            status = m_threadStartState;
        }
    }

    return status;
}

void SyncedSoundStreams::setPlayingOffset(sf::Time timeOffset) {
    Status oldStatus = getStatus(); // Get old playing status
    stop(); // Stop the stream
    // Let the derived class update the current position
    for (auto& data : stream_data) {
        data.onSeek(timeOffset);
    }
    // Restart streaming
    stream_data[music_source_index].m_samplesProcessed = (
        static_cast<sf::Uint64>(
            timeOffset.asSeconds()
            * static_cast<float>(stream_data[music_source_index].m_sampleRate)
        ) * stream_data[music_source_index].m_channelCount
    );
    if (oldStatus == Stopped) {
        return;
    }

    launchStreamingThread(oldStatus);
}

sf::Time SyncedSoundStreams::getPlayingOffset() const {
    if (stream_data[music_source_index].m_sampleRate && stream_data[music_source_index].m_channelCount) {
        ALfloat secs = 0.f;
        alCheck(alGetSourcef(sources[music_source_index], AL_SEC_OFFSET, &secs));
        const auto& d = stream_data[music_source_index];
        return sf::seconds(
            secs
            + static_cast<float>(d.m_samplesProcessed)
            / static_cast<float>(d.m_sampleRate)
            / static_cast<float>(d.m_channelCount)
        );
    } else {
        return sf::Time::Zero;
    }
}

sf::Time SyncedSoundStreams::getPrecisePlayingOffset() const {
    if (this->getStatus() != Status::Playing) {
        return getPlayingOffset();
    } else {
        return (
            getPlayingOffset()
            - (this->alSecOffsetLatencySoft()[1] * this->getPitch())
            + (lag * this->getPitch())
        );
    }
}

void SyncedSoundStreams::setLoop(bool loop) {
    m_loop = loop;
}

bool SyncedSoundStreams::getLoop() const {
    return m_loop;
}

sf::Int64 SyncedSoundStreams::onLoop() {
    music_seek_callback(sf::Time::Zero);
    return 0;
}

void SyncedSoundStreams::setProcessingInterval(sf::Time interval) {
    m_processingInterval = interval;
}

void SyncedSoundStreams::streamData() {
    bool requestStop = false;

    {
        std::scoped_lock lock(m_threadMutex);

        // Check if the thread was launched Stopped
        if (m_threadStartState == Stopped) {
            m_isStreaming = false;
            return;
        }
    }

    // Create the buffers
    for (auto& stream : stream_data) {
        alCheck(alGenBuffers(BufferPerSource, stream.m_buffers.data()));
        for (sf::Int64& bufferSeek : stream.m_bufferSeeks) {
            bufferSeek = NoLoop;
        }
    }

    // Fill the queue
    requestStop = fillQueue();

    // Play the sound
    alCheck(alSourcePlayv(4, sources.data()));

    {
        std::scoped_lock lock(m_threadMutex);

        // Check if the thread was launched Paused
        if (m_threadStartState == Paused)
            alCheck(alSourcePausev(4, sources.data()));
    }

    for (;;) {
        {
            std::scoped_lock lock(m_threadMutex);
            if (!m_isStreaming)
                break;
        }

        // The stream has been interrupted!
        if (getSoundSourceStatus() == Stopped) {
            if (!requestStop) {
                // Just continue
                alCheck(alSourcePlayv(4, sources.data()));
            } else {
                // End streaming
                std::scoped_lock lock(m_threadMutex);
                m_isStreaming = false;
            }
        }

        for (unsigned int source_index = 0; source_index < sources.size(); source_index++) {
            auto& source = sources[source_index];
            auto stream = stream_data[source_index];
            // Get the number of buffers that have been processed (i.e. ready for reuse)
            ALint nbProcessed = 0;
            alCheck(alGetSourcei(source, AL_BUFFERS_PROCESSED, &nbProcessed));

            while (nbProcessed--) {
                // Pop the first unused buffer from the queue
                ALuint buffer;
                alCheck(alSourceUnqueueBuffers(source, 1, &buffer));

                // Find its number
                unsigned int bufferNum = 0;
                for (unsigned int i = 0; i < BufferPerSource; ++i) {
                    if (stream.m_buffers[i] == buffer) {
                        bufferNum = i;
                        break;
                    }
                }

                // Retrieve its size and add it to the samples count
                if (stream.m_bufferSeeks[bufferNum] != NoLoop) {
                    // This was the last buffer before EOF or Loop End: reset the sample count
                    stream.m_samplesProcessed = static_cast<sf::Uint64>(stream.m_bufferSeeks[bufferNum]);
                    stream.m_bufferSeeks[bufferNum] = NoLoop;
                } else {
                    ALint size, bits;
                    alCheck(alGetBufferi(buffer, AL_SIZE, &size));
                    alCheck(alGetBufferi(buffer, AL_BITS, &bits));

                    // Bits can be 0 if the format or parameters are corrupt, avoid division by zero
                    if (bits == 0) {
                        std::cerr << "Bits in sound stream are 0: make sure that the audio format is not corrupt "
                            << "and initialize() has been called correctly" << std::endl;

                        // Abort streaming (exit main loop)
                        std::scoped_lock lock(m_threadMutex);
                        m_isStreaming = false;
                        requestStop = true;
                        break;
                    } else {
                        stream.m_samplesProcessed += static_cast<sf::Uint64>(size / (bits / 8));
                    }
                }

                // Fill it and push it back into the playing queue
                if (!requestStop) {
                    if (fillAndPushBuffer(source_index, bufferNum)) {
                        requestStop = true;
                    }
                }
            }

            // Check if any error has occurred
            if (alGetLastError() != AL_NO_ERROR) {
                // Abort streaming (exit main loop)
                std::scoped_lock lock(m_threadMutex);
                m_isStreaming = false;
                break;
            }

            // Leave some time for the other threads if the stream is still playing
            if (getSoundSourceStatus() != Stopped) {
                sleep(m_processingInterval);
            }
        }
    }

    // Stop the playback
    alCheck(alSourceStopv(4, sources.data()));

    // Dequeue any buffer left in the queue
    clearQueue();

    // Reset the playing position
    for (auto stream: stream_data) {
        stream.m_samplesProcessed = 0;
    }

    // Delete the buffers
    for (auto source: sources) {
        alCheck(alSourcei(source, AL_BUFFER, 0));
    }
    for (auto stream: stream_data) {
        alCheck(alDeleteBuffers(BufferPerSource, stream.m_buffers.data()));
    }
    
}

bool SyncedSoundStreams::fillAndPushBuffer(unsigned int source_index, unsigned int bufferNum, bool immediateLoop) {
    bool requestStop = false;
    auto& stream = stream_data[source_index];
    auto& source = sources[source_index];
    // Acquire audio data, also address EOF and error cases if they occur
    Chunk data = {nullptr, 0};
    for (sf::Uint32 retryCount = 0; !stream.onGetData(data) && (retryCount < BufferRetries); ++retryCount) {
        // Check if the stream must loop or stop
        if (!m_loop) {
            // Not looping: Mark this buffer as ending with 0 and request stop
            if (data.samples != nullptr && data.sampleCount != 0)
                stream.m_bufferSeeks[bufferNum] = 0;
            requestStop = true;
            break;
        }

        // Return to the beginning or loop-start of the stream source using onLoop(), and store the result in the buffer seek array
        // This marks the buffer as the "last" one (so that we know where to reset the playing position)
        stream.m_bufferSeeks[bufferNum] = onLoop();

        // If we got data, break and process it, else try to fill the buffer once again
        if (data.samples != nullptr && data.sampleCount != 0) {
            break;
        }

        // If immediateLoop is specified, we have to immediately adjust the sample count
        if (immediateLoop && (stream.m_bufferSeeks[bufferNum] != NoLoop)) {
            // We just tried to begin preloading at EOF or Loop End: reset the sample count
            stream.m_samplesProcessed = static_cast<sf::Uint64>(stream.m_bufferSeeks[bufferNum]);
            stream.m_bufferSeeks[bufferNum] = NoLoop;
        }

        // We're a looping sound that got no data, so we retry onGetData()
    }

    // Fill the buffer if some data was returned
    if (data.samples && data.sampleCount) {
        unsigned int buffer = stream.m_buffers[bufferNum];

        // Fill the buffer
        auto size = static_cast<ALsizei>(data.sampleCount * sizeof(sf::Int16));
        alCheck(alBufferData(buffer, stream.m_format, data.samples, size, static_cast<ALsizei>(stream.m_sampleRate)));

        // Push it into the sound queue
        alCheck(alSourceQueueBuffers(source, 1, &buffer));
    } else {
        // If we get here, we most likely ran out of retries
        requestStop = true;
    }

    return requestStop;
}

bool SyncedSoundStreams::fillQueue() {
    // Fill and enqueue all the available buffers
    bool requestStop = false;
    for (unsigned int source_index = 0; source_index < sources.size(); source_index++) {
        for (unsigned int i = 0; (i < BufferPerSource) && !requestStop; ++i) {
            // Since no sound has been loaded yet, we can't schedule loop seeks preemptively,
            // So if we start on EOF or Loop End, we let fillAndPushBuffer() adjust the sample count
            if (fillAndPushBuffer(source_index, i, (i == 0))) {
                requestStop = true;
            }
        }
    }
    return requestStop;
}

void SyncedSoundStreams::clearQueue() {
    for (auto source: sources) {
        // Get the number of buffers still in the queue
        alCheck(alSourcei(source, AL_BUFFER, 0));
        ALint nbQueued;
        alCheck(alGetSourcei(source, AL_BUFFERS_QUEUED, &nbQueued));

        // Dequeue them all
        ALuint buffer;
        for (ALint i = 0; i < nbQueued; ++i) {
            alCheck(alSourceUnqueueBuffers(source, 1, &buffer));
        }
    }
}

void SyncedSoundStreams::launchStreamingThread(Status threadStartState) {
    {
        std::scoped_lock lock(m_threadMutex);
        m_isStreaming = true;
        m_threadStartState = threadStartState;
    }

    assert(!m_thread.joinable());
    m_thread = std::thread(&SyncedSoundStreams::streamData, this);
}

void SyncedSoundStreams::awaitStreamingThread() {
    // Request the thread to join
    {
        std::scoped_lock lock(m_threadMutex);
        m_isStreaming = false;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool SyncedSoundStreams::openFromFile(const std::filesystem::path& filename) {
    // First stop the music if it was already running
    stop();

    // Open the underlying sound file
    if (!music_data.m_file.openFromFile(filename)) {
        return false;
    }

    // Perform common initializations
    initialize_music();

    return true;
}

sf::Time SyncedSoundStreams::getMusicDuration() const {
    return music_data.m_file.getDuration();
}

SyncedSoundStreams::TimeSpan SyncedSoundStreams::getLoopPoints() const {
    return TimeSpan(
        stream_data[music_source_index].samplesToTime(music_data.m_loopSpan.offset),
        stream_data[music_source_index].samplesToTime(music_data.m_loopSpan.length)
    );
}

void SyncedSoundStreams::setLoopPoints(TimeSpan timePoints) {
    auto& music = stream_data[music_source_index];
    Span<sf::Uint64> samplePoints(
        music.timeToSamples(timePoints.offset),
        music.timeToSamples(timePoints.length)
    );

    // Check our state. This averts a divide-by-zero. GetChannelCount() is cheap enough to use often
    if (music.m_channelCount == 0 || music_data.m_file.getSampleCount() == 0) {
        std::cerr << "SyncedSoundStreams is not in a valid state to assign Loop Points." << std::endl;
        return;
    }

    // Round up to the next even sample if needed
    samplePoints.offset += (music.m_channelCount - 1);
    samplePoints.offset -= (samplePoints.offset % music.m_channelCount);
    samplePoints.length += (music.m_channelCount - 1);
    samplePoints.length -= (samplePoints.length % music.m_channelCount);

    // Validate
    if (samplePoints.offset >= music_data.m_file.getSampleCount()) {
        std::cerr << "LoopPoints offset val must be in range [0, Duration)." << std::endl;
        return;
    }
    if (samplePoints.length == 0) {
        std::cerr << "LoopPoints length val must be nonzero." << std::endl;
        return;
    }

    // Clamp End Point
    samplePoints.length = std::min(samplePoints.length, music_data.m_file.getSampleCount() - samplePoints.offset);

    // If this change has no effect, we can return without touching anything
    if (samplePoints.offset == music_data.m_loopSpan.offset && samplePoints.length == music_data.m_loopSpan.length) {
        return;
    }

    // When we apply this change, we need to "reset" this instance and its buffer

    // Get old playing status and position
    Status oldStatus = getStatus();
    sf::Time oldPos = getPlayingOffset();

    // Unload
    stop();

    // Set
    music_data.m_loopSpan = samplePoints;

    // Restore
    if (oldPos != sf::Time::Zero) {
        setPlayingOffset(oldPos);
    }

    // Resume
    if (oldStatus == Playing) {
        play();
    }
}

bool SyncedSoundStreams::music_is_loaded() const {
    return stream_data[music_source_index].m_format != 0;
}

void SyncedSoundStreams::clear_music() {
    stop();
    music_data.~MusicData();
    stream_data[music_source_index].m_format = 0;
    stream_data[music_source_index].m_channelCount = 0;
    stream_data[music_source_index].m_sampleRate = 0;
    stream_data[music_source_index].m_samplesProcessed = 0;
}

bool SyncedSoundStreams::music_data_callback(Chunk& data) {
    // auto& stream = stream_data[music_source_index];
    std::scoped_lock lock(music_data.m_mutex);

    std::size_t toFill = music_data.m_samples.size();
    sf::Uint64 currentOffset = music_data.m_file.getSampleOffset();
    sf::Uint64 loopEnd = music_data.m_loopSpan.offset + music_data.m_loopSpan.length;

    // If the loop end is enabled and imminent, request less data.
    // This will trip an "onLoop()" call from the underlying SoundStream,
    // and we can then take action.
    if (getLoop() && (music_data.m_loopSpan.length != 0) && (currentOffset <= loopEnd) && (currentOffset + toFill > loopEnd))
        toFill = static_cast<std::size_t>(loopEnd - currentOffset);

    // Fill the chunk parameters
    data.samples = music_data.m_samples.data();
    data.sampleCount = static_cast<std::size_t>(music_data.m_file.read(music_data.m_samples.data(), toFill));
    currentOffset += data.sampleCount;

    // Check if we have stopped obtaining samples or reached either the EOF or the loop end point
    return (
        (data.sampleCount != 0)
        and (currentOffset < music_data.m_file.getSampleCount())
        and (
            not (
                currentOffset == loopEnd
                and music_data.m_loopSpan.length != 0
            )
        )
    );
}

void SyncedSoundStreams::music_seek_callback(sf::Time timeOffset) {
    std::scoped_lock lock(music_data.m_mutex);
    music_data.m_file.seek(timeOffset);
}

sf::Int64 SyncedSoundStreams::music_loop_callback() {
    // Called by underlying SoundStream so we can determine where to loop.
    std::scoped_lock lock(music_data.m_mutex);
    sf::Uint64 currentOffset = music_data.m_file.getSampleOffset();
    if (getLoop() && (music_data.m_loopSpan.length != 0) && (currentOffset == music_data.m_loopSpan.offset + music_data.m_loopSpan.length))
    {
        // Looping is enabled, and either we're at the loop end, or we're at the EOF
        // when it's equivalent to the loop end (loop end takes priority). Send us to loop begin
        music_data.m_file.seek(music_data.m_loopSpan.offset);
        return static_cast<sf::Int64>(music_data.m_file.getSampleOffset());
    }
    else if (getLoop() && (currentOffset >= music_data.m_file.getSampleCount()))
    {
        // If we're at the EOF, reset to 0
        music_data.m_file.seek(0);
        return 0;
    }
    return NoLoop;
}

std::array<sf::Time, 2> SyncedSoundStreams::alSecOffsetLatencySoft() const {
    ALdouble offsets[2];
    alGetSourcedvSOFT(sources[music_source_index], AL_SEC_OFFSET_LATENCY_SOFT, offsets);
    return {sf::seconds(offsets[0]), sf::seconds(offsets[1])};
}

void SyncedSoundStreams::initialize_music() {
    // Compute the music positions
    music_data.m_loopSpan.offset = 0;
    music_data.m_loopSpan.length = music_data.m_file.getSampleCount();

    // Resize the internal buffer so that it can contain 1 second of audio samples
    music_data.m_samples.resize(
        static_cast<std::size_t>(music_data.m_file.getSampleRate())
        * static_cast<std::size_t>(music_data.m_file.getChannelCount())
    );

    // Initialize the stream
    initialize_sound_stream(
        stream_data[music_source_index],
        music_data.m_file.getChannelCount(),
        music_data.m_file.getSampleRate()
    );
}

sf::Uint64 SyncedSoundStreams::StreamData::timeToSamples(sf::Time position) const {
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep Int64 as the data type throughout the whole operation.
    return (
        (
            (
                static_cast<sf::Uint64>(
                    position.asMicroseconds()
                )
                * m_sampleRate
                * m_channelCount
            )
            + 500000
        )
        / 1000000
    );
}

sf::Time SyncedSoundStreams::StreamData::samplesToTime(sf::Uint64 samples) const {
    sf::Time position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (m_sampleRate != 0 && m_channelCount != 0) {
        position = sf::microseconds(static_cast<sf::Int64>((samples * 1000000) / (m_channelCount * m_sampleRate)));
    }

    return position;
}

