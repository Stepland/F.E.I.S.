/* UTF8-aware load_from_file functions for SFML objects,
uses nowide under the hood */
#pragma once

#include <filesystem>
#include <optional>

#include <nowide/fstream.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/InputStream.hpp>

namespace feis {
    class UTF8FileStream : public sf::InputStream {
    public:
        UTF8FileStream(const std::filesystem::path& file);
        sf::Int64 read(void* data, sf::Int64 size) override;
        sf::Int64 seek(sf::Int64 position) override;
        sf::Int64 tell() override;
        sf::Int64 getSize() override;
    private:
        nowide::ifstream nowide_stream;
        std::filesystem::path path;
    };

    template<class T>
    bool load_from_file(T& object, const std::filesystem::path& file) {
        UTF8FileStream f{file};
        return object.loadFromStream(f);
    }

    class Music : public sf::Music {
    public:
        bool open_from_file(const std::filesystem::path& file);
    protected:
        std::optional<UTF8FileStream> file_stream;
    };
}