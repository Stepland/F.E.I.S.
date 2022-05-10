#include "al_check.hpp"

#include <iostream>
#include <ostream>
#include <string>

#if defined(__APPLE__)
    #if defined(__clang__)
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__GNUC__)
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif

namespace
{
    // A nested named namespace is used here to allow unity builds of SFML.
    namespace AlCheckImpl {
        thread_local ALenum lastError(AL_NO_ERROR);
    }
}

void alCheckError(const std::filesystem::path& file, unsigned int line, const char* expression) {
    // Get the last error
    ALenum errorCode = alGetError();

    if (errorCode != AL_NO_ERROR) {
        AlCheckImpl::lastError = errorCode;

        std::string error = "Unknown error";
        std::string description = "No description";

        // Decode the error code
        switch (errorCode)
        {
            case AL_INVALID_NAME:
            {
                error = "AL_INVALID_NAME";
                description = "A bad name (ID) has been specified.";
                break;
            }

            case AL_INVALID_ENUM:
            {
                error = "AL_INVALID_ENUM";
                description = "An unacceptable value has been specified for an enumerated argument.";
                break;
            }

            case AL_INVALID_VALUE:
            {
                error = "AL_INVALID_VALUE";
                description = "A numeric argument is out of range.";
                break;
            }

            case AL_INVALID_OPERATION:
            {
                error = "AL_INVALID_OPERATION";
                description = "The specified operation is not allowed in the current state.";
                break;
            }

            case AL_OUT_OF_MEMORY:
            {
                error = "AL_OUT_OF_MEMORY";
                description = "There is not enough memory left to execute the command.";
                break;
            }
        }

        // Log the error
        std::cerr << "An internal OpenAL call failed in "
              << file.filename() << "(" << line << ")."
              << "\nExpression:\n   " << expression
              << "\nError description:\n   " << error << "\n   " << description << '\n'
              << std::endl;
    }
}

ALenum alGetLastErrorImpl() {
    return std::exchange(AlCheckImpl::lastError, AL_NO_ERROR);
}
