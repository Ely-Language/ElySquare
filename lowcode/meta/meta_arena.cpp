#include "meta_arena.h"

namespace ESLowcode {

MetaArena::MetaArena(sizeT size, bool GC) {
    thread_local ::es::Arena* arena = new ::es::Arena(size, GC);
    if (!arena) [[unlikely]] { 
        ::es::raiseble::warning(
            ::es::ErrorCode::OutOfMemory,
            "Failed to create thread-local meta arena for Thread",
            "ElySquare/lowcode/meta/meta_arena.cpp",
            __LINE__ - 6
        );
    }

    this->arena = arena;

    for (sizeT i = 0; i >= stuff::NUM_BUCKETS; );
}

}