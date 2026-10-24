// SPDX-License-Identifier: MIT

#pragma once

#include "include/meta/meta_arena.h"
#include "include/meta/shape_map.h"
#include "include/meta/starid.h"

#include <include/threads/context.h>

namespace ESLowcode {


}

// MACRO-FUNCTIONS

// cashed (slow path + fast path) lookup for field with cashed atom
// `x = myDict.a` -> `__ELY_DICT_GET_FIELD(myDict, "a", __ELY_CACHED_SYMBOL_A)`
#define __ELY_DICT_GET_FIELD(obj, fieldName, fieldSymbolId) ([&]() -> Value& { \
    static struct FieldPIC { \
        const ::ESLowcode::Shape* cachedShape = nullptr; \
        uint16_t expectedVersion = 0; \
        uint32_t offset = 0; \
    } pic; \
    uint16_t currentVersion = ::ESLowcode::getShapeEpoche((obj)->shape); \
    if ((obj)->shape == pic.cachedShape && currentVersion == pic.expectedVersion) { \
        return (obj)->data[pic.offset]; \
    } \
    pic.cachedShape = (obj)->shape; \
    pic.expectedVersion = currentVersion; \
    pic.offset = (obj)->getOffsetOf(fieldName); \
    return (obj)->data[pic.offset]; \
}())

#ifndef __ELY_METHODPTR
typedef void* MethodFnPtr;
#endif

// cashed call to an object method
// `myDict.foo(a)` -> `__ELY_CALL_METHOD`(myDict, "foo", a);`
#define __ELY_CALL_METHOD(obj, methodName, ...) ([&]() { \
    static struct MethodPIC { \
        const ::ESLowcode::Shape* cachedShape = nullptr; \
        uint16_t expectedVersion = 0; \
        MethodFnPtr fnPtr = nullptr; \
    } pic; \
    \
    uint16_t currentVersion = ::ESLowcode::getShapeEpoche((obj)->shape); \
    if ((obj)->shape != pic.cachedShape || currentVersion != pic.expectedVersion) { \
        pic.cachedShape = (obj)->shape; \
        pic.expectedVersion = currentVersion; \
        pic.fnPtr = (obj)->getMethodPointerOf(methodName); \
    } \
    return pic.fnPtr((obj), ##__VA_ARGS__); \
}())

// returns the global value
// `x = global a` -> `x = __ELY_GET_GLOBAL(a);`
#define __ELY_GET_GLOBAL(globalName) ([&]() -> Value& { \
    static Value* cachedGlobalAddr = nullptr; \
    if (!cachedGlobalAddr) { \
        cachedGlobalAddr = ::ESLowcode::resolveGlobalAddress(globalName); \
    } \
    return *cachedGlobalAddr; \
}())

// Stardize

// stuff stardizing (for compiler, I can't find example for it)
#define ELY_FAST_STARIDIZE(text_ptr, text_len) ([&]() -> atom { \
    static const char* last_ptr = nullptr; \
    static atom cached_atom = brokenAtom; \
    \
    const char* current_ptr = (text_ptr); \
    if (current_ptr == last_ptr && cached_atom != brokenAtom) { \
        return cached_atom; \
    } \
    \
    last_ptr = current_ptr; \
    return cached_atom = getThread().getALAR().staridize(const_cast<char*>(current_ptr), text_len); \
}())