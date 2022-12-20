#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H
//<includes>
#include "common_common.h"
#if ENVTYPE == WINDOWS_MSVC
#include <windows.h>
#elif ENVTYPE == POSIX_GCC 
#include <sys/sysinfo.h>
#endif
#include <stdint.h>
// </includes>

/*
int32_t common_threading_get_num_cpu_cores(){
#if ENVTYPE == WINDOWS_MSVC
    DWORD numProcs;
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcs = sysInfo.dwNumberOfProcessors;
    return (int32_t) numProcs;
#elif ENVTYPE == POSIX_GCC
    return get_nprocs();
#endif
 
}
*/

Ptr64b* common_mem_malloc_aligned(size_t size_to_alloc, size_t alignment){
    #if ENVTYPE == WINDOWS_MSVC
    return (Ptr64b*)_aligned_malloc(size_to_alloc, alignment); 
    #elif ENVTYPE == POSIX_GCC
    Ptr64b* target;
    if (0 != posix_memalign((void **)(&target), size_to_alloc, alignment)) {
        return NULL;
    } else {
        return target;
    }
    #endif
}
void common_mem_aligned_free(Ptr64b* ptr){
    #if ENVTYPE == WINDOWS_MSVC
    _aligned_free(ptr); 
    #elif ENVTYPE == POSIX_GCC
    free(ptr); 
    #endif
}

#endif
