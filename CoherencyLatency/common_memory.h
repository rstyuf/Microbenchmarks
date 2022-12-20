#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H
//<includes>
#include "common_common.h"
#if IS_MSVC(ENVTYPE)
#include <windows.h>
#elif IS_GCC(ENVTYPE)
#include <sys/sysinfo.h>
#endif
#include <stdint.h>
// </includes>

/*
int32_t common_threading_get_num_cpu_cores(){
#if IS_MSVC(ENVTYPE)
    DWORD numProcs;
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcs = sysInfo.dwNumberOfProcessors;
    return (int32_t) numProcs;
#elif IS_GCC(ENVTYPE)
    return get_nprocs();
#endif
 
}
*/

Ptr64b* common_mem_malloc_aligned(size_t size_to_alloc, size_t alignment){
    #if IS_MSVC(ENVTYPE)
    return (Ptr64b*)_aligned_malloc(size_to_alloc, alignment); 
    #elif IS_GCC(ENVTYPE)
    Ptr64b* target;
    if (0 != posix_memalign((void **)(&target), size_to_alloc, alignment)) {
        return NULL;
    } else {
        return target;
    }
    #endif
}
void common_mem_aligned_free(Ptr64b* ptr){
    #if IS_MSVC(ENVTYPE)
    _aligned_free(ptr); 
    #elif IS_GCC(ENVTYPE)
    free(ptr); 
    #endif
}

#endif
