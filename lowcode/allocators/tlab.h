#pragma once
#include <utility>
#include <memory>
#include "buddy.h"

#ifndef ESL_THREAD_SIZE
#define ESL_THREAD_SIZE 4 * 1024 * 1024 // 4 MB
#endif

namespace es {

class TLAB {
private:
    BuddyAllocator allocator;

public:
    TLAB(const TLAB&) = delete;
    TLAB& operator=(const TLAB&) = delete;
    TLAB(TLAB&&) noexcept = default;
    TLAB& operator=(TLAB&&) noexcept = default;

    explicit TLAB(size_t capacityBytes) : allocator(capacityBytes) {}

    ~TLAB() {
        allocator.~BuddyAllocator();
    }
    
    [[nodiscard]] inline void* alloc(size_t size) noexcept {
        return allocator.alloc(size);
    }

    template<typename T>
    [[nodiscard]] inline T allocof() noexcept {
        return allocator.allocof<T>();
    }

    inline void free(void* ptr) noexcept {
        allocator.free(ptr);
    }

    template<typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args);

    template <typename T>
    void destroy(T* ptr) noexcept;
};

}