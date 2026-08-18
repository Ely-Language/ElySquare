// panic mechanic
// ===============

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

[[noreturn]] void triggerPanic(
    ErrorCode code,      // Error Code
    const char* message, // Error Message
    const char* file,    // Current source code file
    int line             // Line number
);

// TODO AFTER HANDLES
// void registerPanicCallback(
//     PanicHandlerFunc callback, // ptr: void(*)(ErrorCode, const char*)
// );