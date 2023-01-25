#include "../common/common_common.h"
#include <stdio.h>
#include <stdint.h>
#include "CoherencyLatency.h"
/** "CoherencyLatency.h" Includes
 *      typedef TimerResult (*CoherencyLatencyTestType)(unsigned int, unsigned int, uint64_t, Ptr64b*);
 *      #define DEFAULT_ITERATIONS 10000000;
 *      struct LatencyThreadData_t typedefed to LatencyData
 *      And the function declarations
 * */

int main(int argc, char *argv[]) {
    TimerResult **latencies;
    uint64_t iter = COHERENCYLAT_DEFAULT_ITERATIONS;
    int offsets = 1;
    Ptr64b* bouncyBase;
    CoherencyLatencyTestType test= RunCoherencyBounceTest;

    for (int argIdx = 1; argIdx < argc; argIdx++) {
        if (*(argv[argIdx]) == '-') {
            char* arg = argv[argIdx] + 1;
            if (strncmp(arg, "iterations", 10) == 0) { //todo: Make Case Insensitive (ie _strnicmp on Windows)
                argIdx++;
                iter = atoi(argv[argIdx]);
                fprintf(stderr, "%lu iterations requested\n", iter);
            }
            else if (strncmp(arg, "bounce", 6) == 0) { //todo: Make Case Insensitive (ie _strnicmp on Windows)
                fprintf(stderr, "Bouncy\n");
            }
            else if (strncmp(arg, "owned", 5) == 0) { //todo: Make Case Insensitive (ie _strnicmp on Windows)
                test = RunCoherencyOwnedTest;
                fprintf(stderr, "Using separate cache lines for each thread to write to\n");
            }
            else if (strncmp(arg, "offset", 6) == 0) { //todo: Make Case Insensitive (ie _strnicmp on Windows)
                argIdx++;
                offsets = atoi(argv[argIdx]);
                fprintf(stderr, "Offsets: %d\n", offsets);
            }
        }
    }

    return CoherencyTestMain(offsets, iter, test);
}

int CoherencyTestMain(int offsets, int iterations, CoherencyLatencyTestType test){
    Ptr64b* bouncyBase;
    TimerResult **latencies;
    int numProcs = common_threading_get_num_cpu_cores();
    fprintf(stderr, "Number of CPUs: %u\n", numProcs);

    int ret;
    ret = CoherencyTestAllocateTestBuffers(offsets, (test == RunCoherencyOwnedTest? 1:0), &bouncyBase);
    if (ret < 0) return ret;

    ret = CoherencyTestAllocateResultsBuffers(numProcs, offsets, &latencies);
    if (ret < 0) return ret;
    
    CoherencyTestExecute(numProcs, offsets, iterations, bouncyBase, latencies, test);
    CoherencyTestPrintResults(numProcs, offsets, latencies);
    // cleaning up
    CoherencyTestFreeResultsBuffers(offsets, latencies);
    CoherencyTestFreeTestBuffers(bouncyBase);

}

int CoherencyTestAllocateTestBuffers(int offsets, int additional_cachelines, Ptr64b** BaseAddress){
    
    *BaseAddress = (Ptr64b*)common_mem_malloc_aligned(64 * (offsets + additional_cachelines), 4096); 
    if (*BaseAddress == NULL) {
        fprintf(stderr, "Could not allocate aligned mem\n");
        return -1;
    }
    return 0;
}
void CoherencyTestFreeTestBuffers(Ptr64b* BaseAddress){
    common_mem_aligned_free(BaseAddress);
}

int CoherencyTestAllocateResultsBuffers(int numProcs, int offsets, TimerResult ***latencies){
    *latencies = (TimerResult **) malloc(sizeof(TimerResult*) * offsets); //Allocating array of pointers (one for each offset) to arrays of results
    if (*latencies == NULL) {
        fprintf(stderr, "couldn't allocate result array\n");
        return -1;
    }
    memset(*latencies, 0, sizeof(TimerResult*) * offsets); //? Zeroing the pointers? Why? TODO
    for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        (*latencies)[offsetIdx] = (TimerResult*)malloc(sizeof(TimerResult) * numProcs * numProcs);
        if (*latencies == NULL) {
            fprintf(stderr, "couldn't allocate result sub-array %d\n", offsetIdx);
            return -1;
        }
    }
    return 0;
}
void CoherencyTestFreeResultsBuffers(int offsets, TimerResult **latencies){
    for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        TimerResult* latenciesPtr = latencies[offsetIdx];
        free(latenciesPtr);
        latenciesPtr = NULL;
    }
    free(latencies);
}

void CoherencyTestExecute(int numProcs, int offsets, int iterations, Ptr64b* bouncyBase, TimerResult **latencies, CoherencyLatencyTestType test){
    Ptr64b* bouncy = bouncyBase;
    TimerResult non_result;
    non_result.result = 0;
    for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        bouncy = (Ptr64b*)((char*)bouncyBase + offsetIdx * 64);
        //latencies[offsetIdx] = (TimerResult*)malloc(sizeof(TimerResult) * numProcs * numProcs); // seperated out
        TimerResult *latenciesPtr = latencies[offsetIdx];
        
        // Run all to all, skipping testing a core against itself ofc
        // technically can skip the other way around (start j = i + 1) but meh
        for (int i = 0;i < numProcs; i++) {
            for (int j = 0;j < numProcs; j++) {
                latenciesPtr[j + i * numProcs] = i == j ? non_result : test(i, j, iterations, bouncy);
            }
        }
    }
}


void CoherencyTestPrintResults(int numProcs, int offsets, TimerResult **latencies){
    for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        printf("Cache line offset: %d\n", offsetIdx);
        TimerResult* latenciesPtr = latencies[offsetIdx];

        // print thing to copy to excel
        for (int i = 0;i < numProcs; i++) {
            for (int j = 0;j < numProcs; j++) {
                if (j != 0) printf(",");
                if (j == i) printf("x");
                else printf("%f", latenciesPtr[j + i * numProcs].result); //TODO replace with something generic that allows printing more details if we want. (ie, MSR counts)
            }
            printf("\n");
        }
        //free(latenciesPtr);// Now done seperately
    }
}

/// <summary>
/// Measures latency from one logical processor core to another
/// </summary>
/// <param name="processor1">processor number 1</param>
/// <param name="processor2">processor number 2</param>
/// <param name="iter">Number of iterations</param>
/// <param name="addr_base">aligned mem to bounce around</param>
/// <returns>latency per iteration in ns</returns>
TimerResult RunCoherencyBounceTest(unsigned int processor1, unsigned int processor2, uint64_t iter, Ptr64b* addr_base) {
    TimeThreadData timerthreads[2] = {0};
    LatencyData lat1, lat2;
    TimerResult latency;
    TimerStructure timer;

    *addr_base = 0;
    lat1.iterations = iter;
    lat1.start = 1;
    lat1.target = addr_base;
    timerthreads[0].threadFunc = LatencyTestThread;
    timerthreads[0].pArg = (void*) &lat1;
    timerthreads[0].processorIdx = processor1;

    lat2.iterations = iter;
    lat2.start = 2;
    lat2.target = addr_base;
    timerthreads[1].threadFunc = LatencyTestThread;
    timerthreads[1].pArg = (void*) &lat2;
    timerthreads[1].processorIdx = processor2;
    latency = TimeThreads(timerthreads, 2, &timer);

    common_timer_result_process_iterations(&latency, iter *2);
    fprintf(stderr, "%d to %d: %f ns\n", processor1, processor2, latency.result*2 ); //Lat Multiplied by 2 to get previous behavior
    return latency;

}

TimerResult RunCoherencyOwnedTest(unsigned int processor1, unsigned int processor2, uint64_t iter, Ptr64b* addr_base) {
    
    TimeThreadData timerthreads[2] = {0};
    LatencyData lat1, lat2;
    Ptr64b* target1, * target2;
    TimerResult latency;
    TimerStructure timer;
    if (target1 == NULL) {
        fprintf(stderr, "Target is NULL\n");
        latency.result = -1;
        return latency;
    }
    // drop them on different cache lines
    target1 = addr_base; // this is ok because we allocate one more cache line than neceesary if owned
    target2 = target1 + 8;
    

    *target1 = 1;
    *target2 = 0;
    lat1.iterations = iter;
    lat1.start = 3;
    lat1.target = target1;
    lat1.readTarget = target2;
    timerthreads[0].threadFunc = ReadLatencyTestThread;
    timerthreads[0].pArg = &lat1;
    timerthreads[0].processorIdx = processor1;

    lat2.iterations = iter;
    lat2.start = 2;
    lat2.target = target2;
    lat2.readTarget = target1;
    timerthreads[1].threadFunc = ReadLatencyTestThread;
    timerthreads[1].pArg = (void*) &lat2;
    timerthreads[1].processorIdx = processor2;

    latency = TimeThreads(timerthreads, 2, &timer);
    common_timer_result_process_iterations(&latency, iter *2);
    fprintf(stderr, "%d to %d: %f ns\n", processor1, processor2, latency.result*2 ); //Lat Multiplied by 2 to get previous behavior
    return latency;
}

#if IS_MSVC(ENVTYPE)
#include <intrin.h>
//#include <atomic>
// _InterlockedCompareExchange64 seems to be implemented on all relevant platforms incl 32bit ones. Is there signif. penalty on 32b? Does it matter? TODO
#define EXCHANGE_COMPARE(target, exch, comp) (_InterlockedCompareExchange64(target, exch, comp) == comp)  
// Using atomics below didn't seem to work as intended, so here's a less fun implementation:
#if IS_ISA_X86_64(ENVTYPE)
#define STORE_FENCE() (_ReadWriteBarrier())
//#define STORE_FENCE() (_mm_sfence()) 
#elif IS_ISA_X86_i686(ENVTYPE)
#define STORE_FENCE() (_ReadWriteBarrier())
#elif IS_ISA_AARCH64(ENVTYPE)
#define STORE_FENCE() (__dmb(_ARM64_BARRIER_SY)) //Is this too strict?
#elif IS_ISA_ARM32(ENVTYPE)
#define STORE_FENCE() (__dmb(_ARM_BARRIER_SY))  //Is this too strict?
#else
#define STORE_FENCE() (NOT_IMPLEMENTED_YET("STORE_FENCE", "Implemented in x86, x86-64, Arm aarch64"))  //Might be too strong of a barrier? but at least cross platform. Formerly used asm volatile
#endif
//#define STORE_FENCE() (atomic_thread_fence(memory_order_acq_rel)) //TODO: Can we use release consistency here? If yes, memory_order_release. Also, can't find proof that it works on Arm, but I think so.
//#define STORE_FENCE() (_mm_sfence()) //TODO: Find cross platform function for Windows on ARM 

#elif IS_GCC(ENVTYPE)   
#define EXCHANGE_COMPARE(target, exch, comp) (__sync_bool_compare_and_swap(target, comp, exch)) 
//TODO: might be worth examining the newer __atomic_compare_exchange instead of __sync_bool_compare_and_swap but it's a bit weird so maybe not?
#define STORE_FENCE() (__sync_synchronize())  //Might be too strong of a barrier? but at least cross platform. Formerly used asm volatile
//#define STORE_FENCE() (asm volatile ("mfence" ::: "memory")) //TODO: Find cross platform function for non x86 
#endif

/// <summary>
/// Runs one thread of the latency test. should be run in pairs
/// Always writes to target
/// </summary>
/// <param name="param">Latency test params</param>
/// <returns>next value that would have been written to shared memory</returns>
int LatencyTestThread(void *param) {
    LatencyData *latencyData = (LatencyData *)param;
    uint64_t current = latencyData->start;
    while (current <= 2 * latencyData->iterations) {
        if (EXCHANGE_COMPARE(latencyData->target, current, current - 1)) {
            current += 2;
        }
    }

    return current;
}


/// <summary>
/// Similar thing but tries to not bounce cache line ownership
/// Instead, threads write to different cache lines
/// </summary>
/// <param name="param">Latency test params</param>
/// <returns>next value that would have been written to owned mem</returns>
int ReadLatencyTestThread(void *param) {
    LatencyData* latencyData = (LatencyData*)param;
    uint64_t current = latencyData->start;
    //uint64_t startTsc = __rdtsc();
    volatile Ptr64b* read_target = latencyData->readTarget; // needs to be volatile otherwise gcc optimizes it out
    volatile Ptr64b* write_target = latencyData->target;
    while (current <= 2 * latencyData->iterations) {
        if (*read_target == current - 1) {
            *(write_target) = current;
            current += 2;
            STORE_FENCE();
        }
    }

    return current;
}
