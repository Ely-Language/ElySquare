#include "lowcode/allocators/arena.h"
#include "typing.hpp"

namespace ESLowcode {

namespace stuff {

static constexpr sizeT MIN_BLOCK_SIZE = sizeof(void*); 
static constexpr sizeT ALIGN_QUANTUM = 16;
static constexpr sizeT NUM_BUCKETS = 32;

inline sizeT getBucketIndex(sizeT size) noexcept {
    sizeT aligned = (size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : size;
    return (aligned + ALIGN_QUANTUM - 1) / ALIGN_QUANTUM - 1;
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

    void metarecycle() noexcept;

    template<typename T>
    [[nodiscard]] T* metallocof() noexcept {
        return metalloc(sizeof(T), alignof(T));
    }

};

}