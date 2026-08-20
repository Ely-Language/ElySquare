#include "page_mgr.h"

namespace es {

RuntimePageAllocator::RuntimePageAllocator() {
    ::std::memset(m_allocationBitmap, 0, sizeof(m_allocationBitmap));
    ::std::memset(m_trackedAddresses, 0, sizeof(m_trackedAddresses));
    ::std::memset(m_trackedPageCounts, 0, sizeof(m_trackedPageCounts));
}

RuntimePageAllocator::~RuntimePageAllocator() {
    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (m_trackedAddresses[i] != nullptr) {
            void* address = m_trackedAddresses[i];
            size_t bytesToFree = m_trackedPageCounts[i] * ESL_PAGE_SIZE;

            // DUMP
            #ifdef ESL_DEBUG
                std::cerr << "[ElySquare][DEBUG][Lowcode][RuntimePageAllocator]"
                          << "FOUNDED MEMORY LEAK AT ALLOCATORS LEVEL!\n"; 
            #endif

            ::ESLowcode::freePages(address, bytesToFree);

            m_trackedAddresses[i] = nullptr;
            m_trackedPageCounts[i] = 0;
        }
    }
}

intptr_t RuntimePageAllocator::findFreeSlot() {
    for (size_t i = 0; i < BITMAP_SIZE; i++) {
        if (m_allocationBitmap[i] != 0xFFFFFFFFFFFFFFFFULL) {
            for (size_t bit = 0; bit < 64; bit++) {
                if ((m_allocationBitmap[i] & (1ULL << bit)) == 0) {
                    return (i * 64) + bit;
                }
            }
        }
    }
    return -1;
}

void* RuntimePageAllocator::requestPageChunk(size_t pageCount) {
    if (pageCount == 0) return nullptr;
    
    size_t bytesToAllocate = pageCount * ESL_PAGE_SIZE;

    void* allocatedAddress = ::ESLowcode::allocatePages(bytesToAllocate, PagePermissions::ReadWrite);

    if (allocatedAddress == nullptr) {
        triggerPanic(
            ErrorCode::OutOfMemory,
            "OS denied allocation request",
            "ElySquare/lowcode/allocators/page_mgr.cpp",
            39
        );
    }

    intptr_t slot = findFreeSlot();
    if (slot != -1) {
        size_t wordIdx = slot / 64;
        size_t bitIdx = slot % 64;

        m_allocationBitmap[wordIdx] |= (1ULL << bitIdx);
        m_trackedAddresses[slot] = allocatedAddress;
        m_trackedPageCounts[slot] = pageCount;
    } else {
        ::ESLowcode::freePages(allocatedAddress, bytesToAllocate);
        triggerPanic(
            ErrorCode::OutOfMemory,
            "Out of memory bitmap space",
            "ElySquare/lowcode/allocators/page_mgr.cpp",
            50
        );
    }

    return allocatedAddress;
}

void RuntimePageAllocator::releasePageChunk(void* address, size_t pageCount) {
    if (!address || pageCount == 0) return;

    size_t bytesToFree = pageCount * ESL_PAGE_SIZE;
    bool osSuccess = ESLowcode::freePages(address, bytesToFree);

    if (!osSuccess) {
        triggerPanic(
            ErrorCode::OutOfMemory,
            "Failed to free OS pages",
            "ElySquare/lowcode/allocators/page_mgr.cpp",
            73
        );
    }

    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (m_trackedAddresses[i] == address) {
            m_trackedAddresses[i] = nullptr;
            m_trackedPageCounts[i] = 0;

            size_t wordIdx = i / 64;
            size_t bitIdx = i% 64;
            m_allocationBitmap[wordIdx] &= ~(1ULL << bitIdx);
            break;
        }
    }
}

void poisonPages(void* ptr, size_t pageCount) {}

} // namespace es