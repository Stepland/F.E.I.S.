/* UTF8-aware load_from_file functions for SFML objects,
uses nowide under the hood */
#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include <nowide/fstream.hpp>

#include "utf8_file_input_stream.hpp"

namespace feis {
    /* UTF8-aware wrapper around SFML resource classes that "load" files, i.e.
    resource classes that read the whole file at once when calling
    load_from_file() and thus don't need the file stream to remain available
    after the call to load_from_file() */
    template<class T>
    class UTF8Loader : public T {
    public:
        template<typename... Ts>
        bool load_from_path(const std::filesystem::path& file, const Ts&... args) {
            UTF8FileInputStream f;
            if (not f.open(file)) {
                return false;
            }
            return this->loadFromStream(f, args...);
        }
    };

    class HoldsFileStream {
    public:
        virtual ~HoldsFileStream() = default;
    protected:
        UTF8FileInputStream file_stream;
    };
    
    /* UTF8-aware wrapper around SFML resource classes that "open" files, i.e.
    resource classes that just store the file stream when open_from_path() is
    called and stream the file contents on demand and hence require the file
    stream to remain available for the whole lifetime of the resource */
    template<class T>
    class UTF8Streamer : public HoldsFileStream, public T {
    /* The file_stream is kept in a base class to make sure it is destroyed
    AFTER the SFML class that uses it */
    public:
        bool open_from_path(const std::filesystem::path& file) {
            if (not file_stream.open(file)) {
                return false;
            };
            return this->openFromStream(file_stream);
        }
    };

    /* UTF8-aware wrapper around SFML resource classes that stream file contents
    continously during their lifetime files but somehow still use loadFrom*
    methods instead of openFrom* */
    template<class T>
    class UTF8StreamerUsingLoadFrom : public HoldsFileStream, public T {
    public:
        bool load_from_path(const std::filesystem::path& file) {
            if (not file_stream.open(file)) {
                return false;
            };
            return this->loadFromStream(file_stream);
        }
    };
}