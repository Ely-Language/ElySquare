#include "page_mgr.h"

namespace es {

inline uint32_t PageManager::findFirstZeroBit(uint64_t word) {
    if (word == 0xFFFFFFFFFFFFFFFFULL) {
        ::es::triggerPanic(::es::ErrorCode::OutOfMemory, "Error: invalid word code", "ElySquare/lowcode/allocators/page_mgr.cpp", __LINE__ - 1);
        return 0;
    }
    return static_cast<uint32_t>(std::__countr_zero(~word));
}

PageManager::PageManager() {
    CageSegment* segment = new CageSegment();

    void* start = ::ESLowcode::reservePages(ESLM_MEMORY_CHUNK_SIZE);
    if (start == nullptr) ::es::triggerPanic(::es::ErrorCode::OutOfMemory, "Failed reservation of Cage Segment", "ElySquare/lowcode/allocators/page_mgr.cpp", __LINE__ - 1);
    segment->baseAddress = static_cast<uint8_t*>(start);

    memset(segment->baseAddress, 0, sizeof(segment->bitmap));

    segment->next = nullptr;

    head = segment;
    current = segment;
}

void* PageManager::requestPageChunk(size_t pageCount) {
    if (pageCount == 0) [[unlikely]] return nullptr;

    CageSegment* segment = current;

    while (segment != nullptr) {
        size_t runLength = 0;
        size_t runStart = 0;

        auto commitAndReturn = [&](size_t startIdx) -> void* { // Pages fixation
            for (size_t j = 0; j < pageCount; ++j) {
                size_t idx = startIdx + j;
                segment->bitmap[idx / 64] |= (1ULL << (idx % 64));
            }
            uint8_t* targetPtr = segment->baseAddress + (startIdx * ESL_PAGE_SIZE);
            ::ESLowcode::commitPages(targetPtr, pageCount * ESL_PAGE_SIZE, PagePermissions::ReadWrite);
            return targetPtr;
        };

        for (size_t i = 0; i < BITMAP_WORDS; ++i) {
            uint64_t word = segment->bitmap[i];

            if (word == ~0ULL) { // Fully occupied word
                runLength = 0;
                continue;
            }

            if (word == 0ULL) { // Fully free
                if (runLength == 0) runStart = i * 64;
                runLength += 64;
                
                if (runLength >= pageCount) {
                    return commitAndReturn(runStart);
                }
                continue;
            }

            for (size_t b = 0; b < 64; ) {
                if (((word >> b) & 1) == 0) {
                    uint64_t temp = word >> b;
                    int count = std::__countr_zero(temp);
                    int zeros = (count > (64 - static_cast<int>(b))) ? (64 - static_cast<int>(b)) : count;

                    if (runLength == 0) runStart = i * 64 + b;
                    runLength += zeros;
                    b += zeros;

                    if (runLength >= pageCount) {
                        return commitAndReturn(runStart);
                    }
                } else {
                    uint64_t temp = ~(word >> b);
                    int count = std::__countr_zero(temp);
                    int ones = (count > (64 - static_cast<int>(b))) ? (64 - static_cast<int>(b)) : count;
                    
                    runLength = 0;
                    b += ones;
                }
            }
        }
        segment = segment->next;
    }

    return createNewSegmentAndAllocate(pageCount);
}

void* PageManager::createNewSegmentAndAllocate(size_t pageCount) {
    CageSegment* segment = new CageSegment();

    void* start = ::ESLowcode::reservePages(ESLM_MEMORY_CHUNK_SIZE);
    if (start == nullptr) ::es::triggerPanic(::es::ErrorCode::OutOfMemory, "Failed reservation of Cage Segment", "ElySquare/lowcode/allocators/page_mgr.cpp", __LINE__ - 1);
    segment->baseAddress = static_cast<uint8_t*>(start);

    memset(segment->baseAddress, 0, sizeof(segment->bitmap));

    segment->next = nullptr;

    current->next = segment;
    current = segment;
    
    void* ptr = requestPageChunk(pageCount);
    return ptr;
}

void PageManager::releasePageChunk(void* ptr, size_t pageCount) {
    uint8_t* address = static_cast<uint8_t*>(ptr);

    CageSegment* segment = head;
    while (segment != nullptr) {
        if (address >= segment->baseAddress && address < segment->baseAddress + ESLM_MEMORY_CHUNK_SIZE) [[likely]] {
            size_t startIndex = (address - segment->baseAddress) / ESL_PAGE_SIZE;

            for (size_t i = 0; i < pageCount; i++) {
                size_t wordIndex = (startIndex + i) / 64;
                size_t bitOffset = (startIndex + i) % 64;
                segment->bitmap[wordIndex] &= ~(1ULL << bitOffset);
            }

            ::ESLowcode::decommitPages(address, pageCount * ESL_PAGE_SIZE);
            return;
        }
        segment = segment->next;
    }
    ::es::triggerPanic(::es::ErrorCode::OutOfMemory, "Invalid access to other's memory", "ElySquare/lowcode/allocators/page_mgr.cpp", __LINE__ - 8);
}

PageManager::~PageManager() {
    CageSegment* current = head;
    while (current != nullptr) {
        CageSegment* next = current->next;
        ::ESLowcode::releaseReservedPages(current->baseAddress, ESLM_MEMORY_CHUNK_SIZE);
        delete current;
        current = next;
    }
}



} // namespace es