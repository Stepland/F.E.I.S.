#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include <SFML/Audio/Export.hpp>
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundStream.hpp>

#include "precise_sound_stream.hpp"
#include "utf8_file_input_stream.hpp"

class OpenMusic : public PreciseSoundStream {
public:
    using TimeSpan = sf::Music::Span<sf::Time>;
    OpenMusic(const std::filesystem::path& filename);
    ~OpenMusic() override;
    [[nodiscard]] bool openFromFile(const std::filesystem::path& filename);
    sf::Time getDuration() const;
    TimeSpan getLoopPoints() const;
    void setLoopPoints(TimeSpan timePoints);
    std::int64_t timeToSamples(sf::Time position) const;
    sf::Time samplesToTime(std::int64_t samples) const;

protected:
    [[nodiscard]] bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;
    sf::Int64 onLoop() override;

private:
    void initialize();

    feis::UTF8FileInputStream m_file_input_stream;
    std::int64_t lead_in = 0;
    sf::InputSoundFile m_file;     //!< The streamed music file
    std::vector<sf::Int16> m_samples;  //!< Temporary buffer of samples
    std::recursive_mutex m_mutex;    //!< Mutex protecting the data
    sf::Music::Span<sf::Uint64> m_loopSpan; //!< Loop Range Specifier
};