#include "panic.h"

constexpr int MAX_FRAMES = 64;

namespace es {

[[noreturn]] void triggerPanic(ErrorCode code, const char* message, const char* file, int line) {
    void* stackBuffer[MAX_FRAMES];
    int numcode;
    switch (code) {
        case ErrorCode::Success: numcode = 0; break;
        case ErrorCode::OutOfMemory: numcode = 1; break;
        case ErrorCode::DanglingPointerAccess: numcode = 2; break;
        case ErrorCode::TypeNotRegistered: numcode = 3; break;
        case ErrorCode::MailboxKeyNotFound: numcode = 4; break;
        case ErrorCode::SafepointTimeout: numcode = 5; break;
        case ErrorCode::ContainerContainsOther: numcode = 6; break;
        default: numcode = -1; break;
    }
    // printing error
    std::cerr << "[ElySquare][PANIC][ERROR CODE:" << numcode << "][" << file << ':' << line << "]: " << message << "\n";
// STACKTRACE DUMP
// WIN32
#if defined(ESL_OS_WINDOWS)
    USHORT capturedFrames = CaptureStackBackTrace(0, MAX_FRAMES, stackBuffer, nullptr);
    for (USHORT i = 0; i < capturedFrames; ++i) {
        std::cerr << "[ElySquare][PANIC][" << i << "] Address: " << stackBuffer[i] << "\n";
    }
// POSIX
#elif defined(ESL_OS_LINUX)
    int capturedFrames = backtrace(stackBuffer, MAX_FRAMES);
    char** symbols = backtrace_symbols(stackBuffer, capturedFrames);
    if (symbols != nullptr) {
        for (int i = 0; i < capturedFrames; ++i) {
            std::cerr << "[ElySquare][PANIC][" << i << "] " << symbols[i] << "\n";
        }
        free(symbols);
    } else {
        for (int i = 0; i < capturedFrames; ++i) {
            std::cerr << "[ElySquare][PANIC][" << i << "] Address: " << stackBuffer[i] << "\n";
        }
    }
#endif
    std::cerr << "[ElySquare][PANIC] Stacktrace end";
    std::abort();
}

}