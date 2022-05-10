#pragma once

#include <filesystem>

#include <AL/al.h>
#include <AL/alc.h>
#include <SFML/Config.hpp>

#if defined(__APPLE__)
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif

////////////////////////////////////////////////////////////
/// Let's define a macro to quickly check every OpenAL API call
////////////////////////////////////////////////////////////
#ifdef SFML_DEBUG

    // If in debug mode, perform a test on every call
    // The do-while loop is needed so that alCheck can be used as a single statement in if/else branches
    #define alCheck(expr) do { expr; alCheckError(__FILE__, __LINE__, #expr); } while (false)
    #define alGetLastError alGetLastErrorImpl

#else

    // Else, we don't add any overhead
    #define alCheck(expr) (expr)
    #define alGetLastError alGetError

#endif

void alCheckError(const std::filesystem::path& file, unsigned int line, const char* expression);
ALenum alGetLastErrorImpl();

#if defined(__APPLE__)
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif
#endif
