#include "synced_sound_streams.hpp"

#include <cassert>
#include <initializer_list>
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
#include "../toolbox.hpp"
#include "al_check.hpp"
#include "audio_device.hpp"
#include "imgui.h"
#include "precise_sound_stream.hpp"


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

void SyncedSoundStreams::change_streams(
    std::function<void()> callback,
    std::optional<float> new_pitch
) {
    const auto oldStatus = getStatus();
    const auto position = getPlayingOffset();
    stop();

    callback();

    reload_sources();
    if (new_pitch) {
        setPitch(*new_pitch);
    } else {
        setPitch(pitch);
    }
    setPlayingOffset(position);
    if (oldStatus == sf::SoundSource::Playing) {
        play();
    }
}

void SyncedSoundStreams::update_streams(
    const std::map<std::string, NewStream>& to_add,
    const std::initializer_list<std::string>& to_remove,
    std::optional<float> new_pitch
) {
    change_streams(
        [&](){
            for (const auto& name : to_remove) {
                remove_stream_internal(name);
            }
            for (const auto& [name, new_stream] : to_add) {
                remove_stream_internal(name);
                add_stream_internal(name, new_stream);
            }
        },
        new_pitch
    );
}


void SyncedSoundStreams::add_stream(const std::string& name, NewStream s) {
    change_streams([&](){
        add_stream_internal(name, s);
    });
}

void SyncedSoundStreams::add_stream_internal(const std::string& name, NewStream s) {
    InternalStream internal_stream{s.stream, {}, s.bypasses_openal_pitch};
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

bool SyncedSoundStreams::contains_stream(const std::string& name) const {
    return streams.contains(name);
}

bool SyncedSoundStreams::empty() const {
    return streams.empty();
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

    // Set lag values
    for (const auto& [_, s]: streams) {
        s.stream->lag = s.stream->alSecOffsetLatencySoft()[1];
    }
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


void SyncedSoundStreams::setPlayingOffset(const sf::Time timeOffset) {
    // Get old playing status
    auto oldStatus = getStatus();

    // Stop the stream
    stop();

    const auto pre_pitched_time_offset = timeOffset / pitch;
    // Let the derived class update the current position
    for (auto& [_, s]: streams) {
        s.stream->public_seek_callback(timeOffset);
        // Restart streaming
        if (s.bypasses_openal_pitch) {
            s.buffers.m_samplesProcessed = time_to_samples(
                pre_pitched_time_offset,
                s.buffers.m_sampleRate, 
                s.buffers.m_channelCount
            );
        } else {
            s.buffers.m_samplesProcessed = time_to_samples(
                timeOffset,
                s.buffers.m_sampleRate, 
                s.buffers.m_channelCount
            );
        }
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
    return getPlayingOffset(streams.begin()->second);
}

sf::Time SyncedSoundStreams::getPlayingOffset(const InternalStream& s) const {
    if (not (s.buffers.m_sampleRate && s.buffers.m_channelCount)) {
        return sf::Time::Zero;
    }
    ALfloat secs = 0.f;
    alCheck(alGetSourcef(s.stream->get_source(), AL_SEC_OFFSET, &secs));
    const auto openal_seconds = sf::seconds(secs) + samples_to_time(
        s.buffers.m_samplesProcessed,
        s.buffers.m_sampleRate,
        s.buffers.m_channelCount
    );
    if (s.bypasses_openal_pitch) {
        return openal_seconds * pitch;
    } else {
        return openal_seconds;
    }
}

sf::Time SyncedSoundStreams::getPrecisePlayingOffset() const {
    if (streams.empty()) {
        return sf::Time::Zero;
    }
    return getPrecisePlayingOffset(streams.begin()->second);
}

sf::Time SyncedSoundStreams::getPrecisePlayingOffset(const InternalStream& s) const {
    const auto base = getPlayingOffset(s);
    const auto correction = (
        s.stream->alSecOffsetLatencySoft()[1] - s.stream->lag
    );
    return base - (correction * pitch);
}

void SyncedSoundStreams::setPitch(float new_pitch) {
    pitch = new_pitch;
    for (auto& [_, s] : streams) {
        if (not s.bypasses_openal_pitch) {
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

void SyncedSoundStreams::display_debug() const {
    if (ImGui::BeginTable("SSS debug props", streams.size() + 1, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("");
        for (const auto& [name, _] : streams) {
            ImGui::TableSetupColumn(name.c_str());
        }
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("bypasses OpenAL Pitch");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(fmt::format("{}", s.bypasses_openal_pitch).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("SyncedSoundStreams offset");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(Toolbox::to_string(getPlayingOffset(s)).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("internal stream offset");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(Toolbox::to_string(s.stream->getPlayingOffset()).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("SyncedSoundStreams offset (precise)");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(Toolbox::to_string(getPrecisePlayingOffset(s)).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("internal stream offset (precise)");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(Toolbox::to_string(s.stream->getPrecisePlayingOffset()).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("stream info in .buffers");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(fmt::format("{}Hz * {}ch", s.buffers.m_sampleRate, s.buffers.m_channelCount).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("stream info in .stream");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(fmt::format("{}Hz * {}ch", s.stream->getSampleRate(), s.stream->getChannelCount()).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("AL_PITCH");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ALfloat pitch;
            alCheck(alGetSourcef(s.stream->get_source(), AL_PITCH, &pitch));
            ImGui::TextUnformatted(fmt::format("x{}", pitch).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("AL_SEC_OFFSET");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            ALfloat secs = 0.f;
            alCheck(alGetSourcef(s.stream->get_source(), AL_SEC_OFFSET, &secs));
            ImGui::TextUnformatted(Toolbox::to_string(sf::seconds(secs)).c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("samples_to_time()");
        for (const auto& [_, s] : streams) {
            ImGui::TableNextColumn();
            const auto time = samples_to_time(
                s.buffers.m_samplesProcessed,
                s.buffers.m_sampleRate,
                s.buffers.m_channelCount
            );
            ImGui::TextUnformatted(Toolbox::to_string(time).c_str());
        }
        ImGui::EndTable();
    }
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
        bool stream_requested_stop = false;
        for (unsigned int i = 0; (i < BufferCount) && !stream_requested_stop; ++i) {
            // Since no sound has been loaded yet, we can't schedule loop seeks preemptively,
            // So if we start on EOF or Loop End, we let fillAndPushBuffer() adjust the sample count
            if (fillAndPushBuffer(s, i, (i == 0))) {
                stream_requested_stop = true;
            }
        }
        requestStop = requestStop or stream_requested_stop;
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