// EX-BOXING
// dynamic by tagging first 3 bits
#pragma once
#include <cstdint>
#include <bit>
#include <cstring>

#include "typing.hpp"
#include "lowcode/errors/panic.h"
#include "lowcode/errors/error_codes.h"

#define TAG_MASK 0x7ULL         // 111 - MASK (selects all 3 first bits)
#define PTR_MASK ~0x7ULL
#define EXP_MASK 0x7FF0000000000000ULL

#define TAG_GC_PTR 0x00ULL      // 000 (0) - ptr (big object)
#define TAG_FIXNUM 0x01ULL      // 001 (1) - int
#define TAG_FLTNUM 0x02ULL      // 010 (2) - flt (float)
#define TAG_BOOL_SCALAR 0x03ULL // 011 (3) - bool & short scalars
#define TAG_SSOSTR 0x04ULL      // 100 (4) - short string (<7 ASCII symbols + len)
#define TAG_SYMBOL 0x05ULL      // 101 (5) - HASH identifier (symbol/atom for Shape)
#define TAG_RAWFFI 0x06ULL      // 110 (6) - Raw C/FFI ptr outside GC control
#define TAG_NOTHING 0x07ULL     // 111 (7) - Undefined/Null/void ~= void

namespace ESLowcode {

typedef unsigned long long taggedValue;

enum TypeTag : uint8_t {
    gcPtr = 0,
    fixNum = 1,
    fltNum = 2,
    boolOrScalar = 3,
    ssoStr = 4,
    symbol = 5,
    rawOrFFI = 6,
    nothing = 7
};

inline TypeTag getTypeTagByTag(uint8_t tag) {
    return static_cast<TypeTag>(tag & TAG_MASK);
}

// TAG IDENTIFICATION

inline TypeTag extractTag(taggedValue rawValue) {
    return static_cast<TypeTag>(rawValue & TAG_MASK);
} 

inline bool isGCPointer(taggedValue rawValue) {
    return (rawValue & TAG_MASK) == TAG_GC_PTR;
}

// GC POINTERS MANAGEMENT
typedef void GCObjectHeader;

inline GCObjectHeader* extractGCPointer(taggedValue rawValue) {
    return reinterpret_cast<GCObjectHeader*>(rawValue & PTR_MASK);
}

inline taggedValue boxGCPointer(GCObjectHeader* ptr) {
    auto addr = reinterpret_cast<uintptr_t>(ptr);
#ifdef DEBUG
    if ((addr & TAG_MASK) != 0) {
        ::es::raiseble::warning(
            ::es::ErrorCode::DanglingPointerAccess,
            "alignment of GC ptr isn't 8!",
            "ElySquare/lowcode/meta/exbox_core.h",
            __LINE__
        );
    }
#endif
    return (addr & PTR_MASK) | TAG_GC_PTR;
}

// FLOATS (Lossy 50-bit mantissa floating point boxing)

inline taggedValue boxFloat(flt value) {
    uint64_t bits = ::std::bit_cast<uint64_t>(value);
    return (bits & PTR_MASK) | TAG_FLTNUM;
}

inline flt unboxFlt(taggedValue rawValue) {
    return ::std::bit_cast<flt>(rawValue & PTR_MASK);
}

// SSO & ATOMS

inline taggedValue boxSSO(const char* string, uint8_t length) {
    if (length > 7) {
#ifdef DEBUG
        ::es::raiseble::warning(
            ::es::ErrorCode::ContainerContainsOther,
            "String is too long to be boxed like SSO str",
            "ElySquare/lowcode/meta/exbox_core.h",
            __LINE__
        );
#endif
        length = 7;
    }
    if (!string) return TAG_SSOSTR;

    taggedValue result = (static_cast<taggedValue>(length) << 3) | TAG_SSOSTR;

    if constexpr (std::endian::native == std::endian::little) [[likely]] {
        std::memcpy(reinterpret_cast<char*>(&result) + 1, string, length);
    } else {
        for (uint8_t i = 0; i < length; ++i) {
            result |= (static_cast<taggedValue>(static_cast<uint8_t>(string[i])) << (8 + (i * 8)));
        }
    }

    return result;
}

inline char unboxSSOChar(taggedValue rawValue, uint8_t index) {
    return static_cast<char>((rawValue >> (8 + (index * 8))) & 0xFFULL);
}

inline void unboxSSOString(taggedValue rawValue, char* outBuffer) {
    uint8_t length = static_cast<uint8_t>((rawValue >> 3) & 0x1FULL);

    if constexpr (std::endian::native == std::endian::little) [[likely]] {
        std::memcpy(outBuffer, reinterpret_cast<const char*>(&rawValue) + 1, length);
    } else {
        for (uint8_t i = 0; i < length; ++i) {
            outBuffer[i] = unboxSSOChar(rawValue, i);
        }
    }
    outBuffer[length] = '\0';
}

inline taggedValue boxAtom(atom stringHash) {
    return (static_cast<taggedValue>(stringHash) << 3) | TAG_SYMBOL;
}

inline atom unboxAtom(taggedValue rawValue) {
    return static_cast<uint32_t>(rawValue >> 3);
}

inline taggedValue boxForeign(void* rawCPointer) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(rawCPointer);
    #ifdef DEBUG
    if ((addr & TAG_MASK) != 0) {
        ::es::raiseble::warning(
            ::es::ErrorCode::DanglingPointerAccess,
            "alignment of GC ptr isn't 8!",
            "ElySquare/lowcode/meta/exbox_core.h",
            __LINE__
        );
    }
#endif
    return (addr & PTR_MASK) | TAG_RAWFFI;
}

inline void* unboxForeign(taggedValue rawValue) {
    return reinterpret_cast<void*>(rawValue & PTR_MASK);
}

inline taggedValue makeSentinel(SentinelState state) {
    return (static_cast<taggedValue>(state) << 3) | TAG_NOTHING;
}

// FIXNUM (61-bit signed integer)

inline taggedValue boxFixnum(int64_t value) {
    return (static_cast<taggedValue>(value) << 3) | TAG_FIXNUM;
}

inline int64_t unboxFixnum(taggedValue rawValue) {
    return static_cast<int64_t>(rawValue) >> 3;
}

// BOOL & SCALARS

inline taggedValue boxBool(bool value) {
    return (static_cast<taggedValue>(value) << 3) | TAG_BOOL_SCALAR;
}

inline bool unboxBool(taggedValue rawValue) {
    return static_cast<bool>((rawValue >> 3) & 0x01);
}

// SENTINEL

inline bool isUndefined(taggedValue rawValue) {
    return rawValue == makeSentinel(SentinelState::undefined);
}

inline bool isNull(taggedValue rawValue) {
    return rawValue == makeSentinel(SentinelState::null);
}

} // namespace ESLowcode