#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H
//<includes>
#include "common_common.h"
#if IS_WINDOWS(ENVTYPE)
#include <windows.h>
#elif IS_GCC_POSIX(ENVTYPE)
#include <sys/sysinfo.h>
#endif
#include <stdint.h>
// </includes>

/*
int32_t common_threading_get_num_cpu_cores(){
#if IS_WINDOWS(ENVTYPE)
    DWORD numProcs;
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcs = sysInfo.dwNumberOfProcessors;
    return (int32_t) numProcs;
#elif IS_GCC_POSIX(ENVTYPE)
    return get_nprocs();
#endif
 
}
*/

Ptr64b* common_mem_malloc_aligned(size_t size_to_alloc, size_t alignment){
    #if IS_WINDOWS(ENVTYPE)
    return (Ptr64b*)_aligned_malloc(size_to_alloc, alignment); 
    #elif IS_GCC_POSIX(ENVTYPE)
    Ptr64b* target;
    if (0 != posix_memalign((void **)(&target), size_to_alloc, alignment)) {
        return NULL;
    } else {
        return target;
    }
    #endif
}
void common_mem_aligned_free(Ptr64b* ptr){
    #if IS_WINDOWS(ENVTYPE)
    _aligned_free(ptr); 
    #elif IS_GCC_POSIX(ENVTYPE)
    free(ptr); 
    #endif
}

#endif
