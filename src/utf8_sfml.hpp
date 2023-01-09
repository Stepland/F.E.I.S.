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