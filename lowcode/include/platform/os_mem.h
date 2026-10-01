// OS memory management | Взаимодействие с памятью ОС
// ==================================================
#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef ESL_OS_WINDOWS
    #define ESL_OS_WINDOWS
    #endif
    #include <windows.h>
#else
    #ifdef ESL_OS_LINUX
    #define ESL_OS_LINUX
    #endif
    #include <sys/mman.h>
#endif

// PAGES PERMISSIONS FOR ALLOCATION
enum class PagePermissions { // for allocation
    Read,                    // Read-only page
    ReadExecute,              // Read&Execute-only
    ReadWrite,               // Can't be executed, read and write allowed
    ReadWriteExecute,        // Can be executed, read and written
    NoAccess,                // No Access
};

namespace ESLowcode {

// allocates page in kbytes
// @details it will allocate the closest value to `bytes` (value // 4)
void* allocatePages(size_t bytes, PagePermissions flag);

// reserves page in kbytes
void* reservePages(size_t bytes);

// commits bytes to reserved address
bool commitPages(void* address, size_t bytes, PagePermissions flag);

// decomits bytes from reserved address
bool decommitPages(void* address, size_t bytes);

// returns reserved pages to OS
bool releaseReservedPages(void* address, size_t bytes);

// returns allocated memory pages to system
inline bool freePages(void* address, size_t bytes) {
    return ::ESLowcode::releaseReservedPages(address, bytes);
}

// changes permissions of chosen memory page
bool protectPages(void* address, size_t bytes, PagePermissions flag);

}
