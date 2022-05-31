#include "open_music.hpp"

#include <fstream>
#include <algorithm>
#include <mutex>
#include <ostream>

#include <SFML/Audio/Music.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/Time.hpp>
#include <stdexcept>

#include "al_check.hpp"

#if defined(__APPLE__)
    #if defined(__clang__)
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__GNUC__)
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif

OpenMusic::OpenMusic(const std::filesystem::path& filename) :
    m_file(),
    m_loopSpan(0, 0)
{
    if (not openFromFile(filename)) {
        throw std::runtime_error("Could not open "+filename.string());
    }
}


OpenMusic::~OpenMusic() {
    // We must stop before destroying the file
    stop();
}


bool OpenMusic::openFromFile(const std::filesystem::path& filename) {
    // First stop the music if it was already running
    stop();

    // Open the underlying sound file
    if (!m_file.openFromFile(filename)) {
        return false;
    }

    // Perform common initializations
    initialize();
    return true;
}


sf::Time OpenMusic::getDuration() const {
    return m_file.getDuration();
}


OpenMusic::TimeSpan OpenMusic::getLoopPoints() const {
    return TimeSpan(samplesToTime(m_loopSpan.offset), samplesToTime(m_loopSpan.length));
}


void OpenMusic::setLoopPoints(TimeSpan timePoints) {
    sf::Music::Span<sf::Uint64> samplePoints(timeToSamples(timePoints.offset), timeToSamples(timePoints.length));

    // Check our state. This averts a divide-by-zero. GetChannelCount() is cheap enough to use often
    if (getChannelCount() == 0 || m_file.getSampleCount() == 0) {
        sf::err() << "Music is not in a valid state to assign Loop Points." << std::endl;
        return;
    }

    // Round up to the next even sample if needed
    samplePoints.offset += (getChannelCount() - 1);
    samplePoints.offset -= (samplePoints.offset % getChannelCount());
    samplePoints.length += (getChannelCount() - 1);
    samplePoints.length -= (samplePoints.length % getChannelCount());

    // Validate
    if (samplePoints.offset >= m_file.getSampleCount()) {
        sf::err() << "LoopPoints offset val must be in range [0, Duration)." << std::endl;
        return;
    }
    if (samplePoints.length == 0) {
        sf::err() << "LoopPoints length val must be nonzero." << std::endl;
        return;
    }

    // Clamp End Point
    samplePoints.length = std::min(samplePoints.length, m_file.getSampleCount() - samplePoints.offset);

    // If this change has no effect, we can return without touching anything
    if (samplePoints.offset == m_loopSpan.offset && samplePoints.length == m_loopSpan.length) {
        return;
    }

    // When we apply this change, we need to "reset" this instance and its buffer

    // Get old playing status and position
    Status oldStatus = getStatus();
    sf::Time oldPos = getPlayingOffset();

    // Unload
    stop();

    // Set
    m_loopSpan = samplePoints;

    // Restore
    if (oldPos != sf::Time::Zero)
        setPlayingOffset(oldPos);

    // Resume
    if (oldStatus == Playing)
        play();
}


bool OpenMusic::onGetData(SoundStream::Chunk& data) {
    std::scoped_lock lock(m_mutex);

    std::size_t toFill = m_samples.size();
    sf::Uint64 currentOffset = m_file.getSampleOffset();
    sf::Uint64 loopEnd = m_loopSpan.offset + m_loopSpan.length;

    // If the loop end is enabled and imminent, request less data.
    // This will trip an "onLoop()" call from the underlying SoundStream,
    // and we can then take action.
    if (
        getLoop()
        and (m_loopSpan.length != 0)
        and (currentOffset <= loopEnd)
        and (currentOffset + toFill > loopEnd)
    ) {
        toFill = static_cast<std::size_t>(loopEnd - currentOffset);
    }

    // Fill the chunk parameters
    data.samples = m_samples.data();
    data.sampleCount = static_cast<std::size_t>(m_file.read(m_samples.data(), toFill));
    currentOffset += data.sampleCount;

    // Check if we have stopped obtaining samples or reached either the EOF or the loop end point
    return (data.sampleCount != 0) && (currentOffset < m_file.getSampleCount()) && !(currentOffset == loopEnd && m_loopSpan.length != 0);
}



void OpenMusic::onSeek(sf::Time timeOffset) {
    std::scoped_lock lock(m_mutex);
    m_file.seek(timeOffset);
}


sf::Int64 OpenMusic::onLoop() {
    // Called by underlying SoundStream so we can determine where to loop.
    std::scoped_lock lock(m_mutex);
    sf::Uint64 currentOffset = m_file.getSampleOffset();
    if (getLoop() && (m_loopSpan.length != 0) && (currentOffset == m_loopSpan.offset + m_loopSpan.length))
    {
        // Looping is enabled, and either we're at the loop end, or we're at the EOF
        // when it's equivalent to the loop end (loop end takes priority). Send us to loop begin
        m_file.seek(m_loopSpan.offset);
        return static_cast<sf::Int64>(m_file.getSampleOffset());
    }
    else if (getLoop() && (currentOffset >= m_file.getSampleCount()))
    {
        // If we're at the EOF, reset to 0
        m_file.seek(0);
        return 0;
    }
    return NoLoop;
}


void OpenMusic::initialize() {
    // Compute the music positions
    m_loopSpan.offset = 0;
    m_loopSpan.length = m_file.getSampleCount();

    // Resize the internal buffer so that it can contain 1 second of audio samples
    m_samples.resize(static_cast<std::size_t>(m_file.getSampleRate()) * static_cast<std::size_t>(m_file.getChannelCount()));

    // Initialize the stream
    SoundStream::initialize(m_file.getChannelCount(), m_file.getSampleRate());
}


sf::Uint64 OpenMusic::timeToSamples(sf::Time position) const {
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep Int64 as the data type throughout the whole operation.
    return ((static_cast<sf::Uint64>(position.asMicroseconds()) * getSampleRate() * getChannelCount()) + 500000) / 1000000;
}



sf::Time OpenMusic::samplesToTime(sf::Uint64 samples) const {
    sf::Time position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (getSampleRate() != 0 && getChannelCount() != 0) {
        position = sf::microseconds(static_cast<sf::Int64>((samples * 1000000) / (getChannelCount() * getSampleRate())));
    }

    return position;
}
