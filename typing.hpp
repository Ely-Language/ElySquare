#pragma once

#include <cstdint>


// INT ========================================================================

// 8-bit int
typedef int8_t byte;

// typedef signed short short; // ALREADY EXISTS IN C++ WITH THE SAME WEIGHT

// typedef signed int int; // ALREADY EXISTS IN C++ WITH THE SAME WEIGHT

// 64-bit int
typedef signed long long more;

// unsigned 8-bit int
typedef uint8_t ubyte;

// unsigned 16-bit int
typedef unsigned short ushort;

// unsigned 32-bit int
typedef unsigned int uint;

// unsigned 64-bit int
typedef unsigned long long umore;

// Ely size_t analog
typedef size_t sizeT;

// FLOAT ==========================================================================

// inwork
typedef double noised;

// basic float ely type
typedef double flt;

// STR ============================================================================

// TODO while base level

// NULL ===========================================================================

typedef void sentinel;

// empty value
typedef sentinel null;

// undefined value
typedef sentinel undefined;

enum SentinelState : uint8_t {
    null,
    undefined
};

