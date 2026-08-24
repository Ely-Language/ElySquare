#include "os_mem.h"

namespace ESLowcode {

// returns value that is closest to 4kb
constexpr inline size_t getClosestTo4(size_t count) {
    return (count + 4095) & ~static_cast<size_t>(4095);
}

void* allocatePages(size_t bytes, PagePermissions flag) {
    if (bytes == 0) return nullptr; // can't allocate empty memory

    size_t size = getClosestTo4(bytes);

// Windows path
#ifdef ESL_OS_WINDOWS
    // Protection flags conversation
    DWORD flProtect;
    switch (flag)
    {
    case PagePermissions::Read:
        flProtect = PAGE_READONLY;
        break;
    case PagePermissions::ReadExecute:
        flProtect = PAGE_EXECUTE_READ;
        break;
    case PagePermissions::ReadWrite:
        flProtect = PAGE_READWRITE;
        break;
    case PagePermissions::ReadWriteExecute:
        flProtect = PAGE_EXECUTE_READWRITE;
        break;
    default:
        flProtect = PAGE_NOACCESS;
        break;
    }
    
    // Allocation
    void* ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, flProtect);
    
    // Error case
    if (ptr == nullptr) return nullptr;
#else // LINUX
    // Protection flags conversation
    int prot = PROT_NONE;
    switch (flag)
    {
    case PagePermissions::Read:
        prot = PROT_READ;
        break;
    case PagePermissions::ReadExecute:
        prot = PROT_READ | PROT_EXEC;
        break;
    case PagePermissions::ReadWrite:
        prot = PROT_READ | PROT_WRITE;
        break;
    case PagePermissions::ReadWriteExecute:
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
        break;
    default:
        prot = PROT_NONE;
        break;
    }

    // Allocation via mmap
    void* ptr = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Error case (mmap returns MAP_FAILED on error)
    if (ptr == MAP_FAILED) return nullptr;
#endif
    return static_cast<uint8_t*>(ptr);
}

void* reservePages(size_t bytes) {
    if (bytes == 0) return nullptr; // can't allocate empty memory

    size_t size = getClosestTo4(bytes);

#ifdef ESL_OS_WINDOWS
    DWORD flProtect = PAGE_NOACCESS;
    void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE, flProtect);
    if (ptr == nullptr) return nullptr;
#else
    int prot = PROT_NONE;
    void* ptr = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return nullptr;
#endif
    return static_cast<uint8_t*>(ptr);
}

bool commitPages(void* address, size_t bytes, PagePermissions flag) {
    if (address == nullptr) return false;
    if (bytes == 0) return true;

    size_t size = getClosestTo4(bytes);
#ifdef ESL_OS_WINDOWS
    DWORD flProtect;
    switch (flag)
    {
    case PagePermissions::Read:
        flProtect = PAGE_READONLY;
        break;
    case PagePermissions::ReadExecute:
        flProtect = PAGE_EXECUTE_READ;
        break;
    case PagePermissions::ReadWrite:
        flProtect = PAGE_READWRITE;
        break;
    case PagePermissions::ReadWriteExecute:
        flProtect = PAGE_EXECUTE_READWRITE;
        break;
    default:
        flProtect = PAGE_NOACCESS;
        break;
    }
    void* ptr = VirtualAlloc(address, size, MEM_COMMIT, flProtect);
    return ptr != nullptr;
#else
    int prot = PROT_NONE;
    switch (flag)
    {
    case PagePermissions::Read:
        prot = PROT_READ;
        break;
    case PagePermissions::ReadExecute:
        prot = PROT_READ | PROT_EXEC;
        break;
    case PagePermissions::ReadWrite:
        prot = PROT_READ | PROT_WRITE;
        break;
    case PagePermissions::ReadWriteExecute:
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
        break;
    default:
        prot = PROT_NONE;
        break;
    }
    int result = mprotect(address, size, prot);
    return result == 0;
#endif
}

bool decommitPages(void* address, size_t bytes) {
    if (address == nullptr) return false;
    if (bytes == 0) return true;

    size_t size = getClosestTo4(bytes);
#ifdef ESL_OS_WINDOWS

    return VirtualFree(address, size, MEM_DECOMMIT) != 0;
#else
    int madvizeResult = madvise(address, size, MADV_DONTNEED);
    int result = mprotect(address, size, PROT_NONE);
    return (result == 0 && madvizeResult == 0);
#endif
}

bool releaseReservedPages(void* address, size_t bytes) {
    if (address == nullptr) return false;
    if (bytes == 0) return true;

    #ifdef ESL_OS_WINDOWS
    return (bool)VirtualFree(address, 0, MEM_RELEASE) != 0;
#else
    size_t size = getClosestTo4(bytes);
    return munmap(address, size) == 0;
#endif
}

bool protectPages(void* address, size_t bytes, PagePermissions flag) {
    if (address == nullptr || bytes == 0) return false;

    size_t size = getClosestTo4(bytes);
//WINDOWS
#ifdef ESL_OS_WINDOWS
    // Protection flags conversation
    DWORD flProtect;
    switch (flag)
    {
    case PagePermissions::Read:
        flProtect = PAGE_READONLY;
        break;
    case PagePermissions::ReadExecute:
        flProtect = PAGE_EXECUTE_READ;
        break;
    case PagePermissions::ReadWrite:
        flProtect = PAGE_READWRITE;
        break;
    case PagePermissions::ReadWriteExecute:
        flProtect = PAGE_EXECUTE_READWRITE;
        break;
    default:
        flProtect = PAGE_NOACCESS;
        break;
    }

    DWORD oldProtect;
    return VirtualProtect(address, size, flProtect, &oldProtect) != 0;

// TODO: LINUX
#else
    // Protection flags conversation
    int prot = PROT_NONE;
    switch (flag)
    {
    case PagePermissions::Read:
        prot = PROT_READ;
        break;
    case PagePermissions::ReadExecute:
        prot = PROT_READ | PROT_EXEC;
        break;
    case PagePermissions::ReadWrite:
        prot = PROT_READ | PROT_WRITE;
        break;
    case PagePermissions::ReadWriteExecute:
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
        break;
    default:
        prot = PROT_NONE;
        break;
    }

    // mprotect возвращает 0 при успехе
    return mprotect(address, size, prot) == 0;
#endif
}

}