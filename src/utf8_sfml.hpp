/* UTF8-aware load_from_file functions for SFML objects,
uses nowide under the hood */
#pragma once

#include <SFML/System/FileInputStream.hpp>
#include <filesystem>
#include <optional>

#include <nowide/fstream.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/FileInputStream.hpp>

#include "utf8_file_input_stream.hpp"

namespace feis {
    /* UTF8-aware wrapper around SFML resource classes that "load" files, i.e.
    resource classes that read the whole file at once when calling
    load_from_file() and thus don't need the file stream to remain available
    after the call to load_from_file() */
    template<class T>
    class LoadFromPathMixin : public T {
    public:
        bool load_from_path(const std::filesystem::path& file) {
            UTF8FileInputStream f;
            if (not f.open(file)) {
                return false;
            }
            return this->loadFromStream(f);
        }
    };

    /* UTF8-aware wrapper around SFML resource classes that "open" files, i.e.
    resource classes that just store the file stream when open_from_path() is
    called and stream the file contents on demand and hence require the file
    stream to remain available for the whole lifetime of the resource */
    template<class T>
    class HoldFileStreamMixin : public T {
    public:
        bool open_from_path(const std::filesystem::path& file) {
            if (not file_stream.open(file)) {
                return false;
            };
            return this->openFromStream(file_stream);
        }
    protected:
        UTF8FileInputStream file_stream;
    };
}