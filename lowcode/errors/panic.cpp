#include "panic.h"

constexpr int MAX_FRAMES = 64;

[[noreturn]] void triggerPanic(ErrorCode code, const char* message, const char* file, int line) {
    void* stackBuffer[MAX_FRAMES];
    // printing error
    std::cerr << "[ElySquare][PANIC][" << file << ':' << line << "]: " << message << "\n";
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