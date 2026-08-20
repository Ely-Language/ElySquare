// virtual pages manager
// ======================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>

#include "platform/atomics.h"
#include "platform/os_mem.h"

#include "errors/error_codes.h"
#include "errors/panic.h"

PagePermissions DEFAULT_PAGE_PERMISSION = PagePermissions::ReadWriteExecute;

#ifndef ESLM_MAX_PAGE_COUNT
    #define ESLM_MAX_PAGE_COUNT 65536
#endif
#ifndef ESL_PAGE_SIZE
    #define ESL_PAGE_SIZE 4096
#endif

namespace es { // ElySquare main namespace

// ElySquare runtime page allocator
class RuntimePageAllocator {
private:
    static constexpr size_t MAX_PAGES = ESLM_MAX_PAGE_COUNT; // so we can change it
    static constexpr size_t BITMAP_SIZE = MAX_PAGES / 64;

    uint64_t m_allocationBitmap[BITMAP_SIZE];
    void* m_trackedAddresses[MAX_PAGES];
    size_t m_trackedPageCounts[MAX_PAGES];

    uint8_t* m_baseAddress;
    size_t m_totalBytes;

    intptr_t findFreeSlot();

public:
    // Constructor
    RuntimePageAllocator();

    // Destructor
    ~RuntimePageAllocator();

    // Allocate a page
    void* requestPageChunk(size_t pageCount);

    void releasePageChunk(void* address, size_t pageCount);
};

static RuntimePageAllocator ESPageAllocator;

// requests a chunk of memory | дай кусочек памяти
inline void* requestPageChunk(
    size_t pageCount     // count of requested pages (page = 4kb)
) {
ESPageAllocator.requestPageChunk(pageCount);
}

// returns allocated page chunk to OS
inline void releasePageChunk(
    void* ptr,         // address
    size_t pageCount   // count of returned pages
) {
ESPageAllocator.releasePageChunk(ptr, pageCount);
}

// switches pages to NoAccess mode
inline void poisonPages(
    void* ptr,       // base address
    size_t pageCount // count of pages you want protect to
) {
size_t bytesToFree = pageCount * ESL_PAGE_SIZE;
::ESLowcode::protectPages(ptr, bytesToFree, PagePermissions::NoAccess);
}
} // namespace es