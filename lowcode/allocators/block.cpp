#include "block.h"

namespace es {

BlockAllocator::BlockAllocator(size_t elemSize, size_t blocksCount, size_t align) {
    size_t alignedSize = (elemSize + align - 1) & ~(align - 1);
    
    blockSize = (alignedSize < sizeof(FreeBlock)) ? sizeof(FreeBlock) : alignedSize;

    size_t totalSize = blockSize * blocksCount;

    size_t pages = (totalSize + ESL_PAGE_SIZE - 1) / ESL_PAGE_SIZE;
    void* ptr = ::es::ESPageManager.requestPageChunk(pages);
    if (!ptr) [[unlikely]] ::es::triggerPanic(
        ::es::ErrorCode::OutOfMemory,
        "ERROR ALLOCATION OF BLOCK ALLOCATOR",
        "ElySquare/lowcode/allocators/block.cpp",
        __LINE__ - 5
    );
    pagesCount = pages;

    chunk = ptr;
    chunkSize = pages * ESL_PAGE_SIZE;
    
    if (blocksCount == 0) {
        freeList = nullptr;
        return;
    }

    freeList = static_cast<FreeBlock*>(chunk);
    FreeBlock* current = freeList;
    char* memory = static_cast<char*>(chunk);

    for (size_t i = 1; i < blocksCount; i++) {
        FreeBlock* next = reinterpret_cast<FreeBlock*>(memory + i * blockSize);
        current->next = next;
        current = next;
    }

    current->next = nullptr;
}

BlockAllocator::~BlockAllocator() {
    ::es::ESPageManager.releasePageChunk(chunk, pagesCount);
}

[[nodiscard]] void* BlockAllocator::uncheckedAlloc() noexcept {
    if (!freeList) [[unlikely]] return nullptr;
    void* ptr = freeList;
    freeList = freeList->next;
    return ptr;
}

[[nodiscard]] void* BlockAllocator::alloc(size_t size) noexcept {
    if (size > blockSize) [[unlikely]] return nullptr;
    return uncheckedAlloc();
}

void BlockAllocator::free(void* ptr) noexcept {
    if (ptr == nullptr) [[unlikely]] return;
    FreeBlock* casted = static_cast<FreeBlock*>(ptr);
    casted->next = freeList;
    freeList = casted;
    return; 
}

}