#include "buddy.h"

namespace es {

uint32_t BuddyAllocator::ptrToOffset(void* ptr) {
    return static_cast<uint32_t>((uint8_t*)ptr - (uint8_t*)base);
}

BuddyAllocator::FreeBlockHeader* BuddyAllocator::offsetToPtr(uint32_t offset) {
    uint8_t* ptr = (uint8_t*)base + offset;
    return reinterpret_cast<FreeBlockHeader*>(ptr);
}

inline uint32_t BuddyAllocator::getBuddyOffset(uint32_t offset, uint8_t order) {
    return offset ^ (1U << order);
}

uint8_t BuddyAllocator::sizeToOrder(size_t size) {
    size_t totalSize = size + sizeof(BlockHeader);
    
    if (totalSize <= (1U << ::es::staff::MIN_ORDER)) {
        return staff::MIN_ORDER;
    }

    uint8_t order = static_cast<uint8_t>(::std::bit_width(totalSize - 1));

    return ::std::min(order, staff::MAX_ORDER);
}

constexpr inline size_t BuddyAllocator::getIndexByOrder(uint8_t order) {
    return order - staff::MIN_ORDER;
}

void BuddyAllocator::pushFreeBlock(uint32_t offset, uint8_t order) {
    const size_t idx = getIndexByOrder(order);
    uint32_t oldHeadOffset = bins[idx];
    FreeBlockHeader* block = offsetToPtr(offset);
    
    // freedom for this block!
    block->is_free = true;
    block->prev_offset = staff::NULL_OFFSET;
    block->next_offset = oldHeadOffset;
    block->order = order;

    if (oldHeadOffset != staff::NULL_OFFSET) [[likely]] {
        FreeBlockHeader* oldHead = offsetToPtr(oldHeadOffset);
        oldHead->prev_offset = offset;
    }

    // statistic
    bins[idx] = offset;

    activeBinsBitmap |= (1U << idx);
    return;
}

void BuddyAllocator::removeFreeBlock(uint32_t offset, uint8_t order) {
    FreeBlockHeader* block = offsetToPtr(offset);
    
    if (block->prev_offset != staff::NULL_OFFSET) [[likely]] {
        FreeBlockHeader* prev = offsetToPtr(block->prev_offset);
        prev->next_offset = block->next_offset;
    } else {
        const size_t idx = getIndexByOrder(order);
        bins[idx] = block->next_offset;

        if (bins[idx] == staff::NULL_OFFSET) [[unlikely]] {
            activeBinsBitmap &= ~(1U << idx);
        }
    }

    if (block->next_offset != staff::NULL_OFFSET) [[likely]] {
        FreeBlockHeader* next = offsetToPtr(block->next_offset);
        next->prev_offset = block->prev_offset;
    }

    block->is_free = false;
    return;
}

uint32_t BuddyAllocator::splitBlock(uint32_t offset, uint8_t initialOrder, uint8_t targetOrder) {
    for (uint8_t order = initialOrder - 1; order >= targetOrder; order--) {
        auto* block = offsetToPtr(offset);
        block->order = order;
        uint32_t buddyOffset = getBuddyOffset(offset, order);
        pushFreeBlock(buddyOffset, order);
    }
    return offset;
}

void BuddyAllocator::mergeBlock(uint32_t offset, uint8_t order) {
    if (order >= staff::MAX_ORDER) { pushFreeBlock(offset, order); return; }

    uint32_t buddyOffset = getBuddyOffset(offset, order);

    const bool IsBuddyInArena = buddyOffset < (1U << staff::MAX_ORDER);
    
    if (IsBuddyInArena) {
        auto* buddy = offsetToPtr(buddyOffset);

        const bool isBuddyFree = buddy->is_free != 0;
        const bool hasBuddySameOrder = buddy->order == order;

        if (isBuddyFree && hasBuddySameOrder) {
            removeFreeBlock(buddyOffset, order);

            uint32_t newOffset = ::std::min(offset, buddyOffset);

            mergeBlock(newOffset, order + 1);
            return;    
        }
    }

    pushFreeBlock(offset, order);
}

// PUBLIC METHODS =====================================

BuddyAllocator::BuddyAllocator(size_t size) {
    pages = (size + ESL_PAGE_SIZE - 1) / ESL_PAGE_SIZE;
    if (pages == 0) pages = 1;
    void* chunkptr = ::es::ESPageManager.requestPageChunk(pages);
    if (chunkptr == nullptr) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Failed to allocate BuddyAllocator",
            "ElySquare/lowcode/allocators/buddy.cpp",
            __LINE__ - 7
        );
    }

    base = static_cast<uint8_t*>(chunkptr);

    for (uint8_t i = 0; i < staff::BIN_COUNT - 1; i++) {
        bins[i] = staff::NULL_OFFSET;
    }

    activeBinsBitmap = 0;

    uint8_t startOrder = sizeToOrder((pages * ESL_PAGE_SIZE));
    pushFreeBlock(0, startOrder);
}

[[nodiscard]] void* BuddyAllocator::alloc(size_t size) noexcept {
    bool weCantPutIt = size + sizeof(BlockHeader) > (1U << staff::MAX_ORDER);
    if (size == 0 || weCantPutIt) return nullptr;
    
    uint8_t targetOrder = sizeToOrder(size);

    // looking for a bin
    size_t targetIdx = getIndexByOrder(targetOrder);

    uint32_t mask = activeBinsBitmap & (~0U << targetIdx);
    if (mask == 0) return nullptr;

    uint32_t foundIdx = ::std::countr_zero(mask);
    uint32_t foundOrder = foundIdx + staff::MIN_ORDER;

    // allocating
    uint32_t offset = bins[foundIdx];
    removeFreeBlock(offset, foundOrder);

    if (foundOrder > targetOrder) splitBlock(offset, foundOrder, targetOrder);

    auto* block = offsetToPtr(offset);
    block->is_free = false;

    return (uint8_t*)base + offset + sizeof(BlockHeader);
}

void BuddyAllocator::free(void* ptr) noexcept {
    if (ptr == nullptr) return;

    BlockHeader* header = reinterpret_cast<BlockHeader*>((uint8_t*)ptr - sizeof(BlockHeader));

    uint32_t offset = ptrToOffset(header);
    mergeBlock(offset, header->order);

    return;
}

BuddyAllocator::~BuddyAllocator() {
    ::es::ESPageManager.releasePageChunk(base, pages);
}

}