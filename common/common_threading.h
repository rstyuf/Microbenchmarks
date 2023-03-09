#ifndef COMMON_THREADING_H
#define COMMON_THREADING_H
//<includes>
#include "common_common.h"
#if IS_WINDOWS(ENVTYPE)
#include <windows.h>
#include <winbase.h>
#elif IS_GCC_POSIX(ENVTYPE) 
#include <sys/sysinfo.h>
#include <pthread.h>

#define AFFINITY_PTHREAD (false);
#define gettid() syscall(SYS_gettid)

#endif
#include <stdint.h>
// </includes>
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


bool common_threading_set_affinity(int core) { // Sets affinity for the current thread
#if IS_WINDOWS(ENVTYPE)
    HANDLE cur_thread =  GetCurrentThread();
    DWORD_PTR mask = (1ULL << (uint64_t)core);
    if (mask == 0) {
        fprintf(stderr, "unable to set thread affinity to %d (mask ==0)\n", core);
        return false;
    }

    int rc = SetThreadAffinityMask(cur_thread, mask);
    if (rc == 0){ 
        fprintf(stderr, "unable to set thread affinity to %d (SetThreadAffinityMask error: %d)\n", GetLastError());
        return false;
    }
    return true;
#elif IS_GCC_POSIX(ENVTYPE)
    int rc;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

#if AFFINITY_PTHREAD == true
    pthread_t thread = pthread_self();
    rc = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
#else /*if AFFINITY_PTHREAD == false*/
    rc = sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpuset);
#endif /*AFFINITY_PTHREAD*/
    if (rc != 0) {
        fprintf(stderr, "unable to set thread affinity to %d\n", core);
    }
#else
    NOT_IMPLEMENTED_YET("common_threading_set_affinity", "Implemented on Windows and Linux only.");
#endif
}

typedef struct time_threads_data_t{
    int ((*threadFunc)(void *));
    void* pArg;
    int processorIdx;
#if IS_GCC_POSIX(ENVTYPE) && (defined(NEW_PT_TIMETHREADS))    
    pthread_barrier_t* barrier;
#endif
} TimeThreadData;

#if IS_WINDOWS(ENVTYPE)
DWORD WINAPI ThreadFunctionRunner(LPVOID param) {
    TimeThreadData *arg_holder = (TimeThreadData *)param;
    return arg_holder->threadFunc(arg_holder->pArg);
}
#elif IS_GCC_POSIX(ENVTYPE)
void* ThreadFunctionRunner(void* param){
    TimeThreadData *arg_holder = (TimeThreadData *)param;
    common_threading_set_affinity(arg_holder->processorIdx);
    //fprintf(stderr, "thread %ld set affinity %d\n", gettid(), arg_holder->processorIdx);
    #if IS_GCC(ENVTYPE) && (defined(NEW_PT_TIMETHREADS))    
    pthread_barrier_wait(arg_holder->barrier);
    #endif
    arg_holder->threadFunc(arg_holder->pArg);
    pthread_exit(NULL);
}
#endif


// Need to check if TimeThreads can be used generically in other places where threads are used, and maybe in that case it can be made more generic to different thread counts.
// We can take the iter (and relevant print) out of the function, and then turn processorX and latX into two arrays, along with an int/size_t len which represents how many.

#if IS_WINDOWS(ENVTYPE)

TimerResult TimeThreads( TimeThreadData* thread_datas,
                         int thread_count,
                         TimerStructure* timer) {

    TimerResult timer_result;

    HANDLE* testThreads = (HANDLE*)malloc(thread_count * sizeof(HANDLE));
    DWORD* tids = (DWORD*)malloc(thread_count * sizeof(DWORD));


    for (int i= 0; i < thread_count; i++){
        testThreads[i] = CreateThread(NULL, 0, ThreadFunctionRunner, &(thread_datas[i]), CREATE_SUSPENDED, &tids[i]);
        if (testThreads[i] == NULL) {
            fprintf(stderr, "Failed to create test threads %d on core %d \n", i, thread_datas[i].processorIdx);
            // Need better way to do this but for now:
            free(testThreads);
            free(tids);
            // Todo: Close all handles
            timer_result.result = -1;
            return timer_result;
        }
        SetThreadAffinityMask(testThreads[i], 1ULL << (uint64_t)thread_datas[i].processorIdx);
    }

    common_timer_start(timer);
    for (int i= 0; i < thread_count; i++){
        ResumeThread(testThreads[i]);
    }
    WaitForMultipleObjects(thread_count, testThreads, TRUE, INFINITE);
    
    
    common_timer_end(timer, &timer_result);
    
    for (int i= 0; i < thread_count; i++){
        CloseHandle(testThreads[i]);

    }
    free(testThreads);
    free(tids);
    return timer_result;
}

#elif IS_GCC_POSIX(ENVTYPE) && !(defined(NEW_PT_TIMETHREADS))

//TODO: Consider implementing more like the Windows implementation, using either pthread barriers or some other waiting system.
TimerResult TimeThreads( TimeThreadData* thread_datas,
                         int thread_count,
                         TimerStructure* timer) {
    TimerResult timer_result;
    pthread_t* testThreads = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
    int* rc = (int*)malloc(thread_count * sizeof(int));
    
    common_timer_start(timer);
    for (int i= 0; i < thread_count; i++){
        rc[i] = pthread_create(&testThreads[i], NULL, ThreadFunctionRunner, (void *)&(thread_datas[i]));
            if (rc[i] != 0 ) {
            fprintf(stderr, "Failed to create test threads\n");
            // Need better way to do this but for now:
            // Also, to close threads we've already opened
            timer_result.result = -1;
            return timer_result;
        }
    }
    for (int i= 0; i < thread_count; i++){
        pthread_join(testThreads[i], NULL);
    }    
    common_timer_end(timer, &timer_result);    
    return timer_result;
}

#elif IS_GCC_POSIX(ENVTYPE) && (defined(NEW_PT_TIMETHREADS))

TimerResult TimeThreads( TimeThreadData* thread_datas,
                         int thread_count,
                         TimerStructure* timer) {
    TimerResult timer_result;
    pthread_t* testThreads = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
    int* rc = (int*)malloc(thread_count * sizeof(int));

    pthread_barrier_t start_barrier; // might be worth having an end barrier too, not sure how much time it takes to close threads and if that might skew results.
    pthread_barrier_init(&start_barrier, NULL, thread_count + 1 ); 
   
    for (int i= 0; i < thread_count; i++){
        thread_datas[i].barrier =  &start_barrier;

        rc[i] = pthread_create(&testThreads[i], NULL, ThreadFunctionRunner, (void *)&(thread_datas[i]));
        if (rc[i] != 0 ) {
            fprintf(stderr, "Failed to create test threads\n");
            // Need better way to do this but for now:
            // Also, to close threads we've already opened
            timer_result.result = -1;
            return timer_result;
        }
    }

    common_timer_start(timer); // Need to check if this is right. If we start timing before the wait, we could get stuck at barrrier for a while. But after we could start timer way too late.
    pthread_barrier_wait(&start_barrier);
    
    for (int i= 0; i < thread_count; i++){
        pthread_join(testThreads[i], NULL);
    }    
    common_timer_end(timer, &timer_result);    
    
    pthread_barrier_destroy(&start_barrier);

    for (int i= 0; i < thread_count; i++){
        free(testThreads);
        free(rc);  
    }     
    return timer_result;
}
#else
TimerResult TimeThreads(unsigned int processor1,
                       unsigned int processor2,
                       uint64_t iter, 
                       void* lat1, 
                       void* lat2,
                       int (*threadFunc)(void *)) {
    NOT_IMPLEMENTED_YET("TimeThreads", "Implemented on MSVC and GCC");
}
#endif



#endif
