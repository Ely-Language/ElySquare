// panic mechanic
// ===============
#pragma once

#include <iostream>
#include "error_codes.h"

#if defined(_WIN32) || defined(_WIN64)
    #ifndef ESL_OS_WINDOWS
    #define ESL_OS_WINDOWS
    #include <windows.h>
    #endif
#else
    #ifdef ESL_OS_LINUX
    #define ESL_OS_LINUX
    #include <execinfo.h>
    #include <unistd.h>
    #endif
#endif

namespace es {

// For u, my dear user ♥
namespace raiseble {
    [[noreturn]] inline void panic(
        ErrorCode code,      // Error Code
        const char* message, // Error Message
        const char* file,    // Current source code file
        int line             // Line number
    ) { ::es::triggerPanic(code, message, file, line); }

    inline void warning (
        ErrorCode code,      // Error Code
        const char* message, // Error Message
        const char* file,    // Current source code file
        int line             // Line number
    ) { ::es::raiseWarning(code, message, file, line); }
}

// writting error message & dump to the console
[[noreturn]] void triggerPanic(
    ErrorCode code,      // Error Code
    const char* message, // Error Message
    const char* file,    // Current source code file
    int line             // Line number
);

void raiseWarning(
    ErrorCode code,      // Error Code
    const char* message, // Error Message
    const char* file,    // Current source code file
    int line             // Line number
);

// TODO AFTER HANDLES
// void registerPanicCallback(
//     PanicHandlerFunc callback, // ptr: void(*)(ErrorCode, const char*)
// );

}