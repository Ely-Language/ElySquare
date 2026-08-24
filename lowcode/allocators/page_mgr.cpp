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
    CageSegment* atCheck = current;

    while (atCheck != nullptr) {
        for (size_t i = 0; i < BITMAP_WORDS; i++) {
            if (atCheck->bitmap[i] == 0xFFFFFFFFFFFFFFFFULL) continue;
            uint32_t bitIdx = findFirstZeroBit(atCheck->bitmap[i]);
            uint32_t startIndex = (i * 64) + bitIdx;

            // check if we can use current segment
            if (startIndex + pageCount > CAGE_PAGES_COUNT) [[unlikely]] break;
            
            bool isFree = true;
            for (size_t j = 0; j < pageCount; j++) {
                size_t wordIndex = (startIndex + j) / 64;
                size_t bitOffset = (startIndex + j) % 64;
                if ((atCheck->bitmap[wordIndex] & (1ULL << bitOffset)) != 0) {
                    isFree = false;
                    break;
                }
            }

            if (isFree) [[likely]] {
                for (size_t j = 0; j < pageCount; j++) {
                    size_t wordIndex = (startIndex + j) / 64;
                    size_t bitOffset = (startIndex + j) % 64;
                    atCheck->bitmap[wordIndex] |= (1ULL << bitOffset);
                }

                uint8_t* targetPtr = atCheck->baseAddress + (startIndex * ESL_PAGE_SIZE);
                ::ESLowcode::commitPages(targetPtr, pageCount * ESL_PAGE_SIZE, PagePermissions::ReadWrite);
                return targetPtr;
            }
        }

        atCheck = atCheck->next;
    }

    createNewSegmentAndAllocate(pageCount);
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