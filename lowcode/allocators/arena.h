// Arena

#include <cstddef>
#include <cstdint>
#include <utility>
#include <new>
#include <memory>
#include <string_view>
#include "allocators/page_mgr.h"
#include "errors/panic.h"
#include "errors/error_codes.h"

namespace es {

// Arena Allocator
// ================
// lets you allocate a large memory block, that can be cleared only fully.
// @brief has a fixed size, you can't delete only one object in it, only full Arena
class Arena {
private:
    
    struct Chunk {
        std::byte* data; // start
        size_t capacity; // size of it
        size_t offset;   // how many bytes used
        size_t pages;
        Chunk* next;     // (expand mechanic)

        explicit Chunk(size_t bytes) : offset(0), next(nullptr) {
            pages = (bytes + ESL_PAGE_SIZE - 1) / ESL_PAGE_SIZE;
            if (pages == 0) pages = 1;

            capacity = pages * ESL_PAGE_SIZE;
            data = static_cast<std::byte*>(::es::ESPageManager.requestPageChunk(pages));
        }

        ~Chunk() {
            if (data) {
                ::es::ESPageManager.releasePageChunk(data, pages);
            }
        }
    };

    Chunk* head;
    Chunk* current;
    size_t defaultSize;

    inline void expandByNewChunk(size_t size);

    inline void clearChunks();

    static constexpr size_t alignForward(size_t address, size_t alignment) {
        return (address + alignment - 1) & ~(alignment - 1);
    }

public:

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) noexcept;
    Arena& operator=(Arena&&) noexcept;

    explicit Arena(size_t defaultChunkSize = 1048576ULL);
    ~Arena();

    // Allocate memory in this arena.
    [[nodiscard]] void* alloc(
        size_t bytes,                                  // Size of the allocation.
        size_t alignment = alignof(::std::max_align_t) // byte-alignment (default - `std::max_align_t`)
    ) noexcept;

    // Allocate and nullify.
    [[nodiscard]] void* allocZeroed(
        size_t bytes,                                  // Size of the allocation.
        size_t alignment = alignof(::std::max_align_t) // byte-alignment (default - `std::max_align_t`)
    ) noexcept;

    // Clear all Arena with all its contents.
    void clear() noexcept;

    template<typename T>
    // Allocate memory from a specific type.
    [[nodiscard]] T* allocof(size_t count = 1) noexcept {
        return static_cast<T*>(alloc(sizeof(T), alignof(T)));
    }
};

}