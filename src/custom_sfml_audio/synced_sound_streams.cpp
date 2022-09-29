#include "synced_sound_streams.hpp"

#include <boost/math/constants/constants.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>

#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Audio/SoundStream.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <variant>

#include "../variant_visitor.hpp"
#include "al_check.hpp"
#include "audio_device.hpp"
#include "precise_sound_stream.hpp"

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


void InternalStream::clear_queue() {
    // Get the number of buffers still in the queue
    ALint nbQueued;
    const auto source = stream->get_source();
    alCheck(alGetSourcei(source, AL_BUFFERS_QUEUED, &nbQueued));

    // Dequeue them all
    ALuint buffer;
    for (ALint i = 0; i < nbQueued; ++i) {
        alCheck(alSourceUnqueueBuffers(source, 1, &buffer));
    }
}

SyncedSoundStreams::SyncedSoundStreams() :
    m_thread(),
    m_threadMutex(),
    m_threadStartState(sf::SoundSource::Stopped),
    m_isStreaming(false),
    m_loop(false),
    m_processingInterval(sf::milliseconds(10))
{}

SyncedSoundStreams::~SyncedSoundStreams() {
    // Stop the sound if it was playing

    // Wait for the thread to join
    awaitStreamingThread();
}

void SyncedSoundStreams::change_streams(std::function<void()> callback) {
    const auto oldStatus = getStatus();
    const auto position = getPlayingOffset();
    stop();

    callback();

    reload_sources();
    setPlayingOffset(position);
    setPitch(pitch);
    if (oldStatus == sf::SoundSource::Playing) {
        play();
    }
}

void SyncedSoundStreams::update_streams(std::map<std::string, NewStream> new_streams) {
    change_streams([&](){
        for (const auto& [name, new_stream] : new_streams) {
            if (contains_stream(name)) {
                remove_stream_internal(name);
            }
            add_stream_internal(name, new_stream);
        }
    });
}


void SyncedSoundStreams::add_stream(const std::string& name, NewStream s) {
    change_streams([&](){
        add_stream_internal(name, s);
    });
}

void SyncedSoundStreams::add_stream_internal(const std::string& name, NewStream s) {
    InternalStream internal_stream{s.stream, {}, s.reconstruct_on_pitch_change};
    internal_stream.buffers.m_channelCount = s.stream->getChannelCount();
    internal_stream.buffers.m_sampleRate = s.stream->getSampleRate();
    internal_stream.buffers.m_format = AudioDevice::getFormatFromChannelCount(s.stream->getChannelCount());
    streams.emplace(name, internal_stream);
}

void SyncedSoundStreams::remove_stream(const std::string& name) {
    change_streams([&](){
        remove_stream_internal(name);
    });
}

void SyncedSoundStreams::remove_stream_internal(const std::string& name) {
    if (streams.contains(name)) {
        streams.at(name).clear_queue();
    }
    streams.erase(name);
}

bool SyncedSoundStreams::contains_stream(const std::string& name) {
    return streams.contains(name);
}

void SyncedSoundStreams::play() {
    // Check if the sound parameters have been set
    for (const auto& [_, s]: streams) {
        if (s.buffers.m_format == 0) {
            std::cerr << "Failed to play audio stream: sound parameters have not been initialized (call initialize() first)" << std::endl;
            return;
        }
    }

    bool isStreaming = false;
    sf::SoundSource::Status threadStartState = sf::SoundSource::Stopped;

    {
        std::scoped_lock lock(m_threadMutex);

        isStreaming = m_isStreaming;
        threadStartState = m_threadStartState;
    }


    if (isStreaming && (threadStartState == sf::SoundSource::Paused)) {
        // If the sound is paused, resume it
        std::scoped_lock lock(m_threadMutex);
        m_threadStartState = sf::SoundSource::Playing;
        alCheck(alSourcePlayv(sources.size(), sources.data()));
        return;
    } else if (isStreaming && (threadStartState == sf::SoundSource::Playing)) {
        // If the sound is playing, stop it and continue as if it was stopped
        stop();
    } else if (!isStreaming && m_thread.joinable()) {
        // If the streaming thread reached its end, let it join so it can be restarted.
        // Also reset the playing offset at the beginning.
        stop();
    }

    // Start updating the stream in a separate thread to avoid blocking the application
    launchStreamingThread(sf::SoundSource::Playing);
}



void SyncedSoundStreams::pause() {
    // Handle pause() being called before the thread has started
    {
        std::scoped_lock lock(m_threadMutex);

        if (!m_isStreaming) {
            return;
        }

        m_threadStartState = sf::SoundSource::Paused;
    }

    alCheck(alSourcePausev(sources.size(), sources.data()));
}



void SyncedSoundStreams::stop()
{
    // Wait for the thread to join
    awaitStreamingThread();

    // Move to the beginning
    for (auto& [_, s] : streams) {
        s.stream->public_seek_callback(sf::Time::Zero);
    }
}



sf::SoundSource::Status SyncedSoundStreams::getStatus() const
{
    if (streams.empty()) {
        return sf::SoundSource::Stopped;
    }
    auto status = streams.begin()->second.stream->getStatus();

    // To compensate for the lag between play() and alSourceplay()
    if (status == sf::SoundSource::Stopped) {
        std::scoped_lock lock(m_threadMutex);

        if (m_isStreaming)
            status = m_threadStartState;
    }

    return status;
}


void SyncedSoundStreams::setPlayingOffset(sf::Time timeOffset) {
    // Get old playing status
    auto oldStatus = getStatus();

    // Stop the stream
    stop();

    // Let the derived class update the current position
    for (auto& [_, s]: streams) {
        auto stream_pitch = 1.f;
        if (s.reconstruct_on_pitch_change) {
            stream_pitch = pitch;
        }
        s.stream->public_seek_callback(timeOffset * stream_pitch);
        // Restart streaming
        s.buffers.m_samplesProcessed = timeToSamples(timeOffset, s.buffers.m_sampleRate, s.buffers.m_channelCount);
    }

    if (oldStatus == sf::SoundSource::Stopped) {
        return;
    }

    launchStreamingThread(oldStatus);
}


sf::Time SyncedSoundStreams::getPlayingOffset() const {
    if (streams.empty()) {
        return sf::Time::Zero;
    }
    const auto& s = streams.begin()->second;
    if (not (s.buffers.m_sampleRate && s.buffers.m_channelCount)) {
        return sf::Time::Zero;
    }

    ALfloat secs = 0.f;
    alCheck(alGetSourcef(s.stream->get_source(), AL_SEC_OFFSET, &secs));
    const auto unpitched_seconds = sf::seconds(
        secs
        + static_cast<float>(s.buffers.m_samplesProcessed)
        / static_cast<float>(s.buffers.m_sampleRate)
        / static_cast<float>(s.buffers.m_channelCount)
    );
    if (s.reconstruct_on_pitch_change) {
        return unpitched_seconds * pitch;
    } else {
        return unpitched_seconds;
    }
}

sf::Time SyncedSoundStreams::getPrecisePlayingOffset() const {
    const auto base = getPlayingOffset();
    if (streams.empty()) {
        return base;
    }
    const auto& s = streams.begin()->second;
    if (not (s.buffers.m_sampleRate && s.buffers.m_channelCount)) {
        return base;
    }
    auto stream_pitch = s.stream->getPitch();
    if (s.reconstruct_on_pitch_change) {
        stream_pitch = 1.f;
    }
    const auto correction = (
        (s.stream->alSecOffsetLatencySoft()[1] * stream_pitch)
        - (s.stream->lag * stream_pitch)
    );
    return base - correction;
}

void SyncedSoundStreams::setPitch(float new_pitch) {
    pitch = new_pitch;
    for (auto& [_, s] : streams) {
        if (not s.reconstruct_on_pitch_change) {
            s.stream->setPitch(new_pitch);
        }
    }
}


void SyncedSoundStreams::setLoop(bool loop) {
    m_loop = loop;
}


bool SyncedSoundStreams::getLoop() const {
    return m_loop;
}


void SyncedSoundStreams::setProcessingInterval(sf::Time interval) {
    m_processingInterval = interval;
}


void SyncedSoundStreams::streamData() {
    bool requestStop = false;

    {
        std::scoped_lock lock(m_threadMutex);

        // Check if the thread was launched Stopped
        if (m_threadStartState == sf::SoundSource::Stopped) {
            m_isStreaming = false;
            return;
        }
    }

    // Create the buffers
    for (auto& [_, s]: streams) {
        alCheck(alGenBuffers(s.buffers.m_buffers.size(), s.buffers.m_buffers.data()));
        for (auto& bufferSeek : s.buffers.m_bufferSeeks) {
            bufferSeek = NoLoop;
        }
    }

    // Fill the queue
    requestStop = fillQueues();

    // Play the sound
    alCheck(alSourcePlayv(sources.size(), sources.data()));

    {
        std::scoped_lock lock(m_threadMutex);

        // Check if the thread was launched Paused
        if (m_threadStartState == sf::SoundSource::Paused) {
            alCheck(alSourcePausev(sources.size(), sources.data()));
        }
    }

    while (true) {
        {
            std::scoped_lock lock(m_threadMutex);
            if (!m_isStreaming) {
                break;
            }
        }

        // The stream has been interrupted!
        if (std::any_of(streams.begin(), streams.end(), [](auto& s){
            return s.second.stream->getStatus() == sf::SoundSource::Stopped;
        })) {
            if (!requestStop) {
                // Just continue
                alCheck(alSourcePlayv(sources.size(), sources.data()));
            } else {
                // End streaming
                std::scoped_lock lock(m_threadMutex);
                m_isStreaming = false;
                break;
            }
        }
        
        for (auto& [_, s] : streams) {
            // Get the number of buffers that have been processed (i.e. ready for reuse)
            ALint nbProcessed = 0;
            const auto source = s.stream->get_source();
            alCheck(alGetSourcei(source, AL_BUFFERS_PROCESSED, &nbProcessed));

            while (nbProcessed--) {
                // Pop the first unused buffer from the queue
                ALuint buffer;
                alCheck(alSourceUnqueueBuffers(source, 1, &buffer));

                // Find its number
                unsigned int bufferNum = 0;
                for (unsigned int i = 0; i < BufferCount; ++i) {
                    if (s.buffers.m_buffers[i] == buffer) {
                        bufferNum = i;
                        break;
                    }
                }

                // Retrieve its size and add it to the samples count
                if (s.buffers.m_bufferSeeks[bufferNum] != NoLoop) {
                    // This was the last buffer before EOF or Loop End: reset the sample count
                    s.buffers.m_samplesProcessed = static_cast<sf::Uint64>(s.buffers.m_bufferSeeks[bufferNum]);
                    s.buffers.m_bufferSeeks[bufferNum] = NoLoop;
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
                        s.buffers.m_samplesProcessed += static_cast<sf::Uint64>(size / (bits / 8));
                    }
                }

                // Fill it and push it back into the playing queue
                if (not requestStop) {
                    if (fillAndPushBuffer(s, bufferNum)) {
                        requestStop = true;
                    }
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
        if (std::any_of(streams.begin(), streams.end(), [](auto& s){
            return s.second.stream->getStatus() != sf::SoundSource::Stopped;
        })) {
            sleep(m_processingInterval);
        }
    }

    // Stop the playback
    alCheck(alSourceStopv(sources.size(), sources.data()));

    // Dequeue any buffer left in the queue
    clearQueues();

    
    for (auto& [_, s] : streams) {
        // Reset the playing position
        s.buffers.m_samplesProcessed = 0;

        // Delete the buffers
        alCheck(alSourcei(s.stream->get_source(), AL_BUFFER, 0));
        alCheck(alDeleteBuffers(s.buffers.m_buffers.size(), s.buffers.m_buffers.data()));
    }
}



bool SyncedSoundStreams::fillAndPushBuffer(InternalStream& stream, unsigned int bufferNum, bool immediateLoop) {
    bool requestStop = false;

    // Acquire audio data, also address EOF and error cases if they occur
    sf::SoundStream::Chunk data = {nullptr, 0};
    for (sf::Uint32 retryCount = 0; !stream.stream->public_data_callback(data) && (retryCount < BufferRetries); ++retryCount) {
        // Check if the stream must loop or stop
        if (!m_loop) {
            // Not looping: Mark this buffer as ending with 0 and request stop
            if (data.samples != nullptr && data.sampleCount != 0) {
                stream.buffers.m_bufferSeeks[bufferNum] = 0;
            }
            requestStop = true;
            break;
        }

        // Return to the beginning or loop-start of the stream source using onLoop(), and store the result in the buffer seek array
        // This marks the buffer as the "last" one (so that we know where to reset the playing position)
        stream.buffers.m_bufferSeeks[bufferNum] = stream.stream->public_loop_callback();

        // If we got data, break and process it, else try to fill the buffer once again
        if (data.samples != nullptr && data.sampleCount != 0) {
            break;
        }

        // If immediateLoop is specified, we have to immediately adjust the sample count
        if (immediateLoop && (stream.buffers.m_bufferSeeks[bufferNum] != NoLoop)) {
            // We just tried to begin preloading at EOF or Loop End: reset the sample count
            stream.buffers.m_samplesProcessed = static_cast<sf::Uint64>(stream.buffers.m_bufferSeeks[bufferNum]);
            stream.buffers.m_bufferSeeks[bufferNum] = NoLoop;
        }

        // We're a looping sound that got no data, so we retry onGetData()
    }

    // Fill the buffer if some data was returned
    if (data.samples != nullptr && data.sampleCount != 0) {
        unsigned int buffer = stream.buffers.m_buffers[bufferNum];

        // Fill the buffer
        // Stepland : I don't know why, sometimes data.sampleCount is not a
        // multiple of the number of channels which makes OpenAL error out on
        // the alBufferData call, as a safety measure I trunc it down to the
        // nearest multiple
        data.sampleCount -= data.sampleCount % stream.buffers.m_channelCount;
        auto size = static_cast<ALsizei>(data.sampleCount * sizeof(sf::Int16));
        alCheck(alBufferData(buffer, stream.buffers.m_format, data.samples, size, static_cast<ALsizei>(stream.buffers.m_sampleRate)));

        // Push it into the sound queue
        alCheck(alSourceQueueBuffers(stream.stream->get_source(), 1, &buffer));
    }
    else
    {
        // If we get here, we most likely ran out of retries
        requestStop = true;
    }

    return requestStop;
}



bool SyncedSoundStreams::fillQueues() {
    // Fill and enqueue all the available buffers
    bool requestStop = false;
    for (auto& [_, s] : streams) {
        for (unsigned int i = 0; (i < BufferCount) && !requestStop; ++i) {
            // Since no sound has been loaded yet, we can't schedule loop seeks preemptively,
            // So if we start on EOF or Loop End, we let fillAndPushBuffer() adjust the sample count
            if (fillAndPushBuffer(s, i, (i == 0))) {
                requestStop = true;
            }
        }
    }

    return requestStop;
}



void SyncedSoundStreams::clearQueues() {
    for (auto& [_, s] : streams) {
        s.clear_queue();
    }
}



void SyncedSoundStreams::launchStreamingThread(sf::SoundSource::Status threadStartState) {
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

    if (m_thread.joinable())
        m_thread.join();
}

void SyncedSoundStreams::reload_sources() {
    sources.clear();
    sources.reserve(streams.size());
    for (const auto& [_, s] : streams) {
        sources.push_back(s.stream->get_source());
    }
}