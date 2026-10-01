#include "tlab.h"

namespace es {

template<typename T, typename... Args>
[[nodiscard]] T* TLAB::create(Args&&... args) {
    void* mem = allocator.alloc(sizeof(T));
    if (!mem) [[unlikely]] {
        ::es::raiseble::warning(
            ::es::ErrorCode::OutOfMemory,
            "Failed to allocate memory for class",
            "ElySquare/lowcode/allocators/tlab.h",
            __LINE__ - 6
        );
    }
    return ::new (mem) T(std::forward<Args>(args)...);
}

template <typename T>
void TLAB::destroy(T* ptr) noexcept {
    if (!ptr) [[unlikely]] return;
    std::destroy<T>(ptr);
    allocator.free(ptr);
}

}