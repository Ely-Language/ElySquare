// ElySquare Low-level OS part
// ===========================

#if defined(_WIN32) || defined(_WIN64)
    #ifndef ESL_OS_WINDOWS
    #define ESL_OS_WINDOWS
    #endif
#else
    #ifdef ESL_OS_LINUX
    #define ESL_OS_LINUX
    #endif
#endif
