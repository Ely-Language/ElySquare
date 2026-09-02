// Block (Pool) allocator
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <new>
#include <memory>
#include <string_view>
#include "allocators/page_mgr.h"
#include "errors/panic.h"
#include "errors/error_codes.h"

namespace es {

class BlockAllocator {
private:
    struct FreeBlock {
        FreeBlock* next;
    };

    void* chunk;
    size_t chunkSize;
    size_t blockSize;
    size_t pagesCount;
    FreeBlock* freeList;

public:
    BlockAllocator(const BlockAllocator&) = delete;
    BlockAllocator& operator=(const BlockAllocator&) = delete;
    BlockAllocator(BlockAllocator&&) noexcept;
    BlockAllocator& operator=(BlockAllocator&&) noexcept;

    BlockAllocator(size_t elemSize, size_t blocksCount, size_t align = alignof(::std::max_align_t));

    ~BlockAllocator();

    [[nodiscard]] void* uncheckedAlloc() noexcept;

    [[nodiscard]] void* alloc(size_t size) noexcept;
    
    void free(void* ptr) noexcept;
    
    template<typename T>
    // Allocate memory from a specific type.
    [[nodiscard]] T* allocof() noexcept {
        return static_cast<T*>(alloc(sizeof(T)));
    }
};

}