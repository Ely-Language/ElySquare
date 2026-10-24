// SPDX-License-Identifier: MIT

// major text (len = 8-512) -> "*xxxxxx" (starid)
//  + fast comparation by unique id
//  + can store on stack
//  + can use as a key

#pragma once

#include "include/meta/meta_arena.h"
#include "include/threads/context.h"
#include "backend/include/typing/typing.hpp"

#ifndef _ES_STARID_MAX_COUNT
#define _ES_STARID_MAX_COUNT 256U // ~4kb
#endif

namespace ESLowcode {

namespace stuff {
namespace stringFlags {
    const sizeT GC_MARK_RAW_MASK = 0x8000000000000000ULL;
    const atom STARID_STAR_MASK = 0x80000000;
    const atom STARID_INDEX_MASK = 0x0000FFFF;
}
constexpr char starIDCharset[] = "0123456789abcdefghijklmnopqrstuvwxyz";

inline uint64_t hashString(const char* str, sizeT len) {
    uint64_t hash = 14695981039346656037ULL;
    for (sizeT i = 0; i < len; ++i) {
        hash ^= static_cast<uint64_t>(str[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

}

// Arena-Local Atom Registry
class ALAR {
private:
    struct StarPlace {
        char** ptr;
        // gllllllll... g - GC Mark flag, l = len bit
        sizeT lenAndMark = 0ULL;

        inline sizeT len() const {
            return lenAndMark & ~(stuff::stringFlags::GC_MARK_RAW_MASK);
        }

        inline bool isMarkedAlive() const {
            return (lenAndMark & stuff::stringFlags::GC_MARK_RAW_MASK) != 0;
        }

        inline void setMarkedAlive(bool mark) {
            if (mark) {
                lenAndMark |= stuff::stringFlags::GC_MARK_RAW_MASK;
            } else {
                lenAndMark &= ~stuff::stringFlags::GC_MARK_RAW_MASK;
            }
        }

        inline void setLen(sizeT len) {
            lenAndMark = (len & ~(stuff::stringFlags::GC_MARK_RAW_MASK));
        }
    };

    ::ESLowcode::MetaArena* arena;
    
    StarPlace* placelist;
    umore* hashCache;
    
    ushort starIDMaxCount;
    ubyte* idmap;

    ushort createdPlacesCount = 0;
public:
    ALAR(::ESLowcode::MetaArena* arenaCreatingIn, ushort starIDMaxCount = _ES_STARID_MAX_COUNT);
    ~ALAR() = default;

    atom numToStarId(ushort num);
    ushort starIdToNum(atom starID);
    atom getNewStarId();

    atom getStarIdOf(const char* raw, sizeT length);

    atom staridize(char* text, sizeT length);

    const char* getRaw(atom starId);

    sizeT lenOf(atom starID);

    sizeT sweepPlaces();
};

// checks if raw string is SSO
inline constexpr bool isSSO(sizeT len) {
    return (len < 8);
}


inline atom charToAtom(const char* text) {
    if (!text) return 0;

    if (text[0] == '*') {
        text++;

        ushort num = 0;
        while (*text != '\0') {
            num *= 36;
            char c = *text;
            
            if (c >= '0' && c <= '9') num += (c - '0');
            else if (c >='a' && c <= 'z') num += (c - 'a' + 10);
            else if (c >= 'A' && c <= 'Z') num += (c - 'A' + 10);

            text++;
        }

        return stuff::stringFlags::STARID_STAR_MASK | static_cast<atom>(num);
    }
    
    atom ssoVal = 0;
    ::std::memcpy(&ssoVal, text, ::std::min(size_t(4), ::std::strlen(text)));
    
    ssoVal &= ~stuff::stringFlags::STARID_STAR_MASK;
    return ssoVal;
}

inline void atomToBufferChar(atom a, char* outBuffer, sizeT bufferSize = 8) {
    if (a == brokenAtom) [[unlikely]] {
        return;
    }
    if ((a & stuff::stringFlags::STARID_STAR_MASK) != 0) {
        ushort num = static_cast<ushort>(a & stuff::stringFlags::STARID_INDEX_MASK);

        outBuffer[0] = '*';
        outBuffer[7] = '\0';

        ubyte idx = 6;
        atom n = num;

        do {
            outBuffer[idx--] = stuff::starIDCharset[n % 36]; // Исправлен баг с побитовым & 36 на % 36
            n /= 36;
        } while (n > 0 && idx > 0);

        while (idx > 0) {
            outBuffer[idx--] = '0';
        }
    } else {
        ::std::memcpy(outBuffer, &a, std::min(size_t(4), bufferSize - 1));
        outBuffer[4] = '\0';
    }
}

inline char atomToChar(atom a) {
    char out[8];

    atomToBufferChar(a, out, 8);
    return *out;
}



}