// virtual pages manager
// ======================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <bit>

#include "platform/atomics.h"
#include "platform/os_mem.h"

#include "errors/error_codes.h"
#include "errors/panic.h"

PagePermissions DEFAULT_PAGE_PERMISSION = PagePermissions::ReadWriteExecute;

#ifndef ESLM_MEMORY_CHUNK_SIZE
    #define ESLM_MEMORY_CHUNK_SIZE 8589934592ULL
#endif
#ifndef ESL_PAGE_SIZE
    #define ESL_PAGE_SIZE 4096ULL
#endif
#define CAGE_PAGES_COUNT 2097152ULL
#define BITMAP_WORDS 32768ULL

namespace es { // ElySquare main namespace

class PageManager {
private:
    struct CageSegment {
        uint8_t* baseAddress;
        uint64_t bitmap[BITMAP_WORDS];
        CageSegment* next;
    };
    
    CageSegment* head;
    CageSegment* current;
    
    uint64_t totalReserved;
    uint64_t totalCommited;
    uint64_t totalFree;

    inline uint32_t findFirstZeroBit(uint64_t word);

    void* createNewSegmentAndAllocate(size_t pageCount);

public:
    PageManager();

    ~PageManager();

    void* requestPageChunk(size_t pageCount);

    void releasePageChunk(void* ptr, size_t pageCount);
};

inline PageManager ESPageManager;

}