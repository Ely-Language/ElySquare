#include "arena.h"

namespace es {

inline void Arena::expandByNewChunk(size_t size) {
    Chunk* chunk = new Chunk(size);
    current->next = chunk;
    current = chunk;
    return;
}

inline void Arena::clearChunks() {
    Chunk* chunk = head;
    Chunk* nextchunk = nullptr;
    while (chunk != nullptr) {
        nextchunk = chunk->next;
        delete chunk;
        chunk = nextchunk;
    }
    return;
}

Arena::Arena(size_t defaultChunkSize = 1048576ULL) {
    Chunk* chunk = new Chunk(defaultChunkSize);
    head = chunk;
    current = chunk;
    defaultSize = defaultChunkSize;
}

Arena::~Arena() {
    clearChunks();
}

void* Arena::alloc(size_t bytes, size_t alignment) noexcept {
    Chunk* chunk = current; 

    // Align to
    size_t alignedOffset = (chunk->offset + alignment - 1) & ~(alignment - 1);

    if (alignedOffset + bytes > chunk->capacity) [[unlikely]] {
        if (bytes <= defaultSize) [[likely]] { // Fit it into standard block?
            Chunk* newchunk = new Chunk(defaultSize);
            chunk->next = newchunk;
            chunk = newchunk;
            current = newchunk;
            alignedOffset = (chunk->offset + alignment - 1) & ~(alignment - 1);
        } else { // Ah no we need to make oversized chunk 
            Chunk* oversized = new Chunk(bytes);
            oversized->next = head->next;
            head->next = oversized;
            chunk = oversized;
            alignedOffset = (chunk->offset + alignment - 1) & ~(alignment - 1);
        }
    }
    void* result = chunk->data + alignedOffset;
    chunk->offset = alignedOffset + bytes;
    return result;
}

void Arena::clear() {
    clearChunks();
    Chunk* chunk = new Chunk(defaultSize);
    head = chunk;
    current = chunk;
}

}