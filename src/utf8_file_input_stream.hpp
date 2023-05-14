/*
Custom version of FileInputStream that adds an .open() overload that works with
non-ascii paths
*/

#pragma once

#include <SFML/Config.hpp>
#include <SFML/System/Export.hpp>
#include <SFML/System/InputStream.hpp>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>


namespace feis {
    class UTF8FileInputStream : public sf::InputStream {
    public:
        [[nodiscard]] bool open(const std::filesystem::path& filename);
        [[nodiscard]] sf::Int64 read(void* data, sf::Int64 size) override;
        [[nodiscard]] sf::Int64 seek(sf::Int64 position) override;
        [[nodiscard]] sf::Int64 tell() override;
        sf::Int64 getSize() override;
    private:
        struct FileCloser {
            void operator()(std::FILE* file);
        };
        std::unique_ptr<std::FILE, FileCloser> m_file;
    };
}
