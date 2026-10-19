// SPDX-License-Identifier: MIT

#include "include/meta/meta_arena.h"

namespace ESLowcode {

MetaArena::MetaArena(sizeT size, bool GC) {
    ::es::Arena* arena = new ::es::Arena(size, GC);
    if (!arena) [[unlikely]] { 
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Failed to create thread-local meta arena for Thread",
            "ElySquare/lowcode/meta/meta_arena.cpp",
            __LINE__ - 6
        );
    }

    this->arena = arena;

    for (sizeT i = 0; i < stuff::NUM_BUCKETS; i++) {
        freeListHead[i] = nullptr;
    }
}

MetaArena::~MetaArena() {
    delete arena;
}

void* MetaArena::metalloc(sizeT size, sizeT alignment) noexcept {
    sizeT actualSize = stuff::alignSize(size);
    sizeT bucketIdx = stuff::getBucketIndex(actualSize);

    
    // Cached (FreeLists)
    if (bucketIdx < stuff::NUM_BUCKETS && freeListHead[bucketIdx] != nullptr) {
#ifdef DEBUG
        if (alignment > stuff::ALIGN_QUANTUM) [[unlikely]]
            ::es::raiseble::panic(
                ::es::ErrorCode::InternalError,
                "metalloc: alignment > ALIGN_QUANTUM cannot be served from free list",
                __FILE__, __LINE__
            );
#endif
        FreeList* node = freeListHead[bucketIdx];
        freeListHead[bucketIdx] = node->next;
        return static_cast<void*>(node);
    }

    void* node = arena->alloc(actualSize, alignment);
    return node;
}

void MetaArena::metarecycle(void* ptr, sizeT size) noexcept {
    if (!ptr) {
#ifdef DEBUG
        ::es::raiseble::warning(
            ::es::ErrorCode::DanglingPointerAccess,
            "Invalid attempt to delete empty ptr in thread meta arena",
            "ElySquare/lowcode/meta/meta_arena.cpp",
            __LINE__ - 6
        );
#endif
        return;
    }

    sizeT actualSize = stuff::alignSize(size);
    sizeT bucketIdx = stuff::getBucketIndex(actualSize);

    if (bucketIdx >= stuff::NUM_BUCKETS) {
#ifdef DEBUG
        ::es::raiseble::warning(
            ::es::ErrorCode::InternalError,
            "Block too large for free list, memory not reused",
            __FILE__, __LINE__
        );
#endif
        return;
    }

#ifdef DEBUG
    for (FreeList* n = freeListHead[bucketIdx]; n; n = n->next) {
        if (n == ptr) [[unlikely]]
            ::es::raiseble::panic(
                ::es::ErrorCode::InternalError,
                "metarecycle: double free detected",
                __FILE__, __LINE__
            );
    }
#endif

    FreeList* node = static_cast<FreeList*>(ptr);
    node->next = freeListHead[bucketIdx];
    freeListHead[bucketIdx] = node;
}

}