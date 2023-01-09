#include <cstddef>
#include <cstdio>

typedef FILE* ImFileHandle;
ImFileHandle ImFileOpen(const char* filename, const char* mode);
bool ImFileClose(ImFileHandle file);
std::size_t ImFileGetSize(ImFileHandle file);
std::size_t ImFileRead(void* data, std::size_t size, std::size_t count, ImFileHandle file);
std::size_t ImFileWrite(const void* data, std::size_t size, std::size_t count, ImFileHandle file);