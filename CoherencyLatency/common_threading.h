#ifndef COMMON_THREADING_H
#define COMMON_THREADING_H
//<includes>
#include "common_common.h"
#if ENVTYPE == WINDOWS_MSVC
#include <windows.h>
#elif ENVTYPE == POSIX_GCC 
#include <sys/sysinfo.h>
#endif
#include <stdint.h>
// </includes>

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


#endif
