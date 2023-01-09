#include "imgui_config_feis.hpp"

#include <nowide/cstdio.hpp>


ImFileHandle ImFileOpen(const char* filename, const char* mode) {
    return nowide::fopen(filename, mode);
}

// We should in theory be using fseeko()/ftello() with off_t and _fseeki64()/_ftelli64() with __int64, waiting for the PR that does that in a very portable pre-C++11 zero-warnings way.
bool    ImFileClose(ImFileHandle f)     { return fclose(f) == 0; }
std::size_t   ImFileGetSize(ImFileHandle f)   { long off = 0, sz = 0; return ((off = ftell(f)) != -1 && !fseek(f, 0, SEEK_END) && (sz = ftell(f)) != -1 && !fseek(f, off, SEEK_SET)) ? (std::size_t)sz : (std::size_t)-1; }
std::size_t   ImFileRead(void* data, std::size_t sz, std::size_t count, ImFileHandle f)           { return fread(data, (size_t)sz, (size_t)count, f); }
std::size_t   ImFileWrite(const void* data, std::size_t sz, std::size_t count, ImFileHandle f)    { return fwrite(data, (size_t)sz, (size_t)count, f); }