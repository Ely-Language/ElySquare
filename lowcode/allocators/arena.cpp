#include "arena.h"

namespace es {

inline void Arena::expandByNewChunk(size_t size) {
    Chunk* chunk = new Chunk(size);
    current->next = chunk;
    current = chunk;
}

inline void Arena::clearChunks() {
    Chunk* chunk = head;
    Chunk* nextchunk = nullptr;
    while (chunk != nullptr) {
        nextchunk = chunk->next;
        delete chunk;
        chunk = nextchunk;
    }
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

    size_t alignedOffset = alignForward(chunk->offset, alignment);

    if (alignedOffset + bytes > chunk->capacity) [[unlikely]] {
        size_t nextChunkSize = (bytes <= defaultSize) ? defaultSize : bytes;
        
        Chunk* newchunk = new Chunk(nextChunkSize);
        chunk->next = newchunk;
        current = newchunk;
        chunk = newchunk;
        
        alignedOffset = alignForward(chunk->offset, alignment);
    }
    
    void* result = chunk->data + alignedOffset;
    chunk->offset = alignedOffset + bytes;
    return result;
}

void Arena::popToMarker(Marker marker) noexcept {
    if (!marker.chunk) [[unlikely]] return;

    Chunk* toDelete = marker.chunk->next;
    marker.chunk->next = nullptr;

    while (toDelete != nullptr) {
        Chunk* next = toDelete->next;
        delete toDelete;
        toDelete = next;
    }

    current = marker.chunk;
    current->offset = marker.offset;
}

void Arena::clear() {
    clearChunks();
    Chunk* chunk = new Chunk(defaultSize);
    head = chunk;
    current = chunk;
}

}