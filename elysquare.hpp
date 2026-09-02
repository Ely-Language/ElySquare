#pragma once
// This is ElySquare framework
// MIT License
// User code only, no AI code here!
#ifdef ES_DEBUG
    #define ESL_DEBUG
#endif

// If you can launch ur project because of memory
// just try to set thiw macro
#define ESLM_BASE_MEMORY_CHUNK 16 * 1024 * 1024 * 1024