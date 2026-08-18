#include <stdint.h>

enum class MemoryOrder {
    Acquire,
    Release,
    SequentialConsistency
};

namespace ESLowcode {

bool atomicCompareExchange(volatile uint64_t* destination, uint64_t* expected, uint64_t desired) {

}

int64_t atomicFetchAdd(volatile uint64_t* destination, int64_t value) {

}

void memoryFence(MemoryOrder order) {
    
}

}