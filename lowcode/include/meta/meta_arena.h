#pragma once

#include "lowcode/allocators/arena.h"
#include "typing.hpp"

namespace ESLowcode {

namespace stuff {

static constexpr sizeT ALIGN_QUANTUM = 16;
static constexpr sizeT MIN_BLOCK_SIZE = ALIGN_QUANTUM; 
static constexpr sizeT NUM_BUCKETS = 32;

inline sizeT alignSize(sizeT size) noexcept {
    sizeT actual = (size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : size;
    return (actual + ALIGN_QUANTUM - 1) & ~(ALIGN_QUANTUM - 1);
}

inline sizeT getBucketIndex(sizeT alignedSize) noexcept {
    return (alignedSize / ALIGN_QUANTUM) - 1;
}

}

class MetaArena {
private:
    struct FreeList {
        FreeList* next;
    };

    ::es::Arena* arena;
    
    FreeList* freeListHead[stuff::NUM_BUCKETS];
public:
    MetaArena(sizeT size, bool GC = false);
    ~MetaArena();

    [[nodiscard]] void* metalloc(sizeT size, sizeT alignment = alignof(::std::max_align_t)) noexcept;

    void metarecycle(void* ptr, sizeT size) noexcept;

    template<typename T>
    [[nodiscard]] inline T* metallocof() noexcept {
        return static_cast<T*>(metalloc(sizeof(T), alignof(T)));
    }

    template<typename T>
    inline void metarecycleof(T* ptr) noexcept {
        if (!ptr) {
#ifdef DEBUG
        ::es::raiseble::warning(
            ::es::ErrorCode::DanglingPointerAccess,
            "Invalid attempt to delete empty SPECIFIED ptr in thread meta arena (next warning is the same error)",
            "ElySquare/lowcode/meta/meta_arena.cpp",
            __LINE__ - 6
        );
#endif
            return;
        }
        if constexpr (!::std::is_trivially_destructible_v<T>) {
            ptr->~T();
        }
        metarecycle(static_cast<void*>(ptr), sizeof(T));
    }

};

}