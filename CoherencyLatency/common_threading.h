#ifndef COMMON_THREADING_H
#define COMMON_THREADING_H
//<includes>
#include "common_common.h"
#if ENVTYPE == WINDOWS_MSVC
#include <windows.h>
#elif ENVTYPE == POSIX_GCC 
#include <sys/sysinfo.h>
#include <pthread.h>
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


typedef struct time_threads_func_args_holder_t{
    int ((*threadFunc)(void *));
    void* arg_struct_ptr;
    int coreIdx;
} TimeThreadFuncArgsHolder;

#if ENVTYPE == WINDOWS_MSVC
DWORD WINAPI ThreadFunctionRunner(LPVOID param) {
    TimeThreadFuncArgsHolder *arg_holder = (TimeThreadFuncArgsHolder *)param;
    return arg_holder->threadFunc(arg_holder->arg_struct_ptr);
}
#elif ENVTYPE == POSIX_GCC
#define gettid() syscall(SYS_gettid)
void* ThreadFunctionRunner(void* param){
    TimeThreadFuncArgsHolder *arg_holder = (TimeThreadFuncArgsHolder *)param;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(arg_holder->coreIdx, &cpuset);
    sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpuset);
    //fprintf(stderr, "thread %ld set affinity %d\n", gettid(), arg_holder->coreIdx);
    arg_holder->threadFunc(arg_holder->arg_struct_ptr);
    pthread_exit(NULL);
}
#endif

#if ENVTYPE == WINDOWS_MSVC
TimerResult TimeThreads(unsigned int processor1,
                       unsigned int processor2,
                       uint64_t iter, 
                       void* lat1, 
                       void* lat2,
                       int (*threadFunc)(void *)) {
    TimerStructure timer;
    TimerResult timer_result;
    HANDLE testThreads[2];
    DWORD tid1, tid2;
    TimeThreadFuncArgsHolder funcholder_thread_1, funcholder_thread_2;
    funcholder_thread_1.threadFunc =  threadFunc;
    funcholder_thread_1.arg_struct_ptr = lat1;
    funcholder_thread_1.coreIdx =  processor1;
    funcholder_thread_2.threadFunc =  threadFunc;
    funcholder_thread_2.arg_struct_ptr = lat2;
    funcholder_thread_2.coreIdx =  processor2;

    testThreads[0] = CreateThread(NULL, 0, ThreadFunctionRunner, &funcholder_thread_1, CREATE_SUSPENDED, &tid1);
    testThreads[1] = CreateThread(NULL, 0, ThreadFunctionRunner, &funcholder_thread_2, CREATE_SUSPENDED, &tid2);

    if (testThreads[0] == NULL || testThreads[1] == NULL) {
        fprintf(stderr, "Failed to create test threads\n");
        // Need better way to do this but for now:
        timer_result.per_iter_ns = -1;
        return timer_result;
    }

    SetThreadAffinityMask(testThreads[0], 1ULL << (uint64_t)processor1);
    SetThreadAffinityMask(testThreads[1], 1ULL << (uint64_t)processor2);

    common_timer_start(&timer);
    ResumeThread(testThreads[0]);
    ResumeThread(testThreads[1]);
    WaitForMultipleObjects(2, testThreads, TRUE, INFINITE);
    
    // each thread does interlocked compare and exchange iterations times. We multipy iter count by 2 to get overall count of locked ops
    common_timer_end(&timer, &timer_result, iter*2);

    fprintf(stderr, "%d to %d: %f ns\n", processor1, processor2, timer_result.per_iter_ns*2); //Lat Multiplied by 2 to get previous behavior
    
    CloseHandle(testThreads[0]);
    CloseHandle(testThreads[1]);
    
    return timer_result;
}
#elif ENVTYPE == POSIX_GCC

//TODO: Consider implementing more like the Windows implementation, using either pthread barriers or some other waiting system.
TimerResult TimeThreads(unsigned int processor1,
                       unsigned int processor2,
                       uint64_t iter, 
                       LatencyData lat1, 
                       LatencyData lat2,
                       int (*threadFunc)(void *)) {
    TimerStructure timer;
    TimerResult timer_result;
    pthread_t testThreads[2];
    int t1rc, t2rc;
    void *res1, *res2;
    TimeThreadFuncArgsHolder funcholder_thread_1, funcholder_thread_2;
    funcholder_thread_1.threadFunc =  threadFunc;
    funcholder_thread_1.arg_struct_ptr =  &lat1;
    funcholder_thread_1.coreIdx =  processor1;
    funcholder_thread_2.threadFunc =  threadFunc;
    funcholder_thread_2.arg_struct_ptr =  &lat2;
    funcholder_thread_2.coreIdx =  processor2;
    
    common_timer_start(&timer);
    t1rc = pthread_create(&testThreads[0], NULL, ThreadFunctionRunner, (void *)&funcholder_thread_1);
    t2rc = pthread_create(&testThreads[1], NULL, ThreadFunctionRunner, (void *)&funcholder_thread_2);
    if (t1rc != 0 || t2rc != 0) {
        fprintf(stderr, "Failed to create test threads\n");
        // Need better way to do this but for now:
        timer_result.per_iter_ns = -1;
        return timer_result;
    }

    pthread_join(testThreads[0], &res1);
    pthread_join(testThreads[1], &res2);
    
    // each thread does interlocked compare and exchange iterations times. We multipy iter count by 2 to get overall count of locked ops
    common_timer_end(&timer, &timer_result, iter*2);

    fprintf(stderr, "%d to %d: %f ns\n", processor1, processor2, timer_result.per_iter_ns*2); //Lat Multiplied by 2 to get previous behavior
    
    return timer_result;
}
#endif



#endif
