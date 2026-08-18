#include <stdint.h>

#if defined(_MSC_VER)
    #include <intrin.h>
    #define ESL_COMPILER_MSVC
#elif defined(__GNUC__) || defined(__clang__)
    #define ESL_COMPILER_GCC_CLANG
#endif

enum class MemoryOrder {
    Release,              // Everything up to this line needs to be completed before we will go next
    Acquire,              // Nothing below to this line can be executed before this line
    SequentialConsistency // Stops conveyor of threading until all threads will be synchronized
};

namespace ESLowcode {

// cheks if the value in memory at address of `destination` is equal to `expected`, if true: set this value to `desired`
inline bool atomicCompareExchange(
    volatile uint64_t* destination, // place in memory that we want to check
    uint64_t* expected,             // what are wel looking for at `destination`
    uint64_t desired                // what we would like to put into memory at `destination` if position == `expected`
) {
// MSVC
#if defined(ESL_COMPILER_MSVC)
    uint64_t initial = static_cast<uint64_t>((_InterlockedCompareExchange64(
        reinterpret_cast<volatile __int64*>(destination),
        static_cast<__int64>(desired),
        static_cast<__int64>(*expected)
    ))
    if (initial == *expected) {
        return true;
    } else {
        *expected = initial;
        return false;
    }
#elif defined(ESL_COMPILER_GCC_CLANG)
    return __atomic_compare_exchange_n(
        destination,
        expected,
        desired,
        false,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST
    );
#else
    #ifdef ES_DEBUG_FLAG
        #error "Unsupported compiler"
    #endif
#endif
}

// 1. checks the value in memory at address of `destination`
// 2. adds to value at this address amount of `value`
// 3. returns the old value (before step 2)
inline int64_t atomicFetchAdd(volatile uint64_t* destination, int64_t value) {
#if defined(ESL_COMPILER_MSVC)
    return static_cast<int64_t>(
        _InterlockedExchangeAdd64(
            reinterpret_cast<volatile __int64*>(destination),
            static_cast <__int64>(value)
        )
    );
#elif defined(ESL_COMPILER_GCC_CLANG)
return static_cast<int64_t>(__atomic_fetch_add(destination, value, __ATOMIC_SEQ_CST));
#else
    #ifdef ES_DEBUG_FLAG
        #error "Unsupported compiler"
    #endif
#endif
}

// Very difficult thing
// Lets us manage threading tasks 
void memoryFence(MemoryOrder order) {
#if defined(ESL_COMPILER_MSVC)
    switch (order) {
        case MemoryOrder::Acquire:
            _ReadWriteBarrier(); 
            break;
        case MemoryOrder::Release:
            _ReadWriteBarrier();
            break;
        case MemoryOrder::SequentialConsistency:
            _mm_mfence();
            break;
    }
#elif defined(ESL_COMPILER_GCC_CLANG)
    switch (order) {
        case MemoryOrder::Acquire:
            __atomic_thread_fence(__ATOMIC_ACQUIRE);
            break;
        case MemoryOrder::Release:
            __atomic_thread_fence(__ATOMIC_RELEASE);
            break;
        case MemoryOrder::SequentialConsistency:
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            break;
    }
#else
    #ifdef ES_DEBUG_FLAG
        #error "Unsupported compiler"
    #endif
#endif
}

}