// Buddy allocator
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <new>
#include <memory>
#include <string_view>
#include <bit>
#include "allocators/page_mgr.h"
#include "errors/panic.h"
#include "errors/error_codes.h"

namespace es {

namespace staff {
    constexpr uint8_t MIN_ORDER = 4; // 16 B (2^4)
    constexpr uint8_t MAX_ORDER = 22; // 4 MB (2^22)
    constexpr uint8_t BIN_COUNT = MAX_ORDER - MIN_ORDER + 1;
    constexpr uint32_t NULL_OFFSET = 0xFFFFFFFF;
}

// Allocates required size
// main allocator of EMSTC
class BuddyAllocator {
private:
    // Header of any memory block of it
    struct alignas(2) BlockHeader {
        uint8_t order : 7; // from 4 till 22
        uint8_t is_free : 1; // is this block free?
    }; // 1 byte
    // Instance of allocator's block header for free blocks
    struct FreeBlockHeader : BlockHeader {
        uint32_t next_offset; // offset to the next block
        uint32_t prev_offset; // offset to the previous block
    }; // 9 bytes

    void* base; // start address
    size_t pages; // size of it in pages
    uint32_t bins[::es::staff::BIN_COUNT]; // headers bins
    uint32_t activeBinsBitmap; // bitmap

    uint32_t ptrToOffset(void* ptr); // returns allocator's offset of given ptr
    FreeBlockHeader* offsetToPtr(uint32_t offset); // returns ptr by given offset
    inline uint32_t getBuddyOffset(uint32_t offset, uint8_t order); // returns buddy's offset
    uint8_t sizeToOrder(size_t size); // returns order from element's size
    constexpr inline size_t getIndexByOrder(uint8_t order); // returns index of first header with this order

    void pushFreeBlock(uint32_t offset, uint8_t order); // adds free block
    void removeFreeBlock(uint32_t offset, uint8_t order); // removes free block

    uint32_t splitBlock(uint32_t offset, uint8_t initialOrder, uint8_t targetOrder); // splits block to 2 parts
    void mergeBlock(uint32_t offset, uint8_t order); // merges 2 blocks to 1

public:

    BuddyAllocator(const BuddyAllocator&) = delete;
    BuddyAllocator& operator=(const BuddyAllocator&) = delete;
    // BuddyAllocator(BuddyAllocator&&) noexcept;
    // BuddyAllocator& operator=(BuddyAllocator&&) noexcept;

    BuddyAllocator(size_t size);

    ~BuddyAllocator();
    
    // Allocates with fixed size
    [[nodiscard]] void* alloc(size_t size) noexcept;

    // Frees allocated pointer
    void free(void* ptr) noexcept;

    // Allocates in BuddyAllocator
    template<typename T>
    [[nodiscard]] T* allocof() noexcept {
        return alloc(sizeof(T));
    }
};

}