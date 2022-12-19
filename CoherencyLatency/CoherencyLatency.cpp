#include <stdio.h>
#include <stdint.h>
#include <intrin.h>
//#include <windows.h>
#include "common_timer.h"
#include "common_threading.h"

#define ITERATIONS 10000000;

TimerResult RunTest(unsigned int processor1, unsigned int processor2, uint64_t iter);
TimerResult RunOwnedTest(unsigned int processor1, unsigned int processor2, uint64_t iter);
DWORD WINAPI LatencyTestThread(LPVOID param);
DWORD WINAPI ReadLatencyTestThread(LPVOID param);

LONG64* bouncyBase;
LONG64* bouncy;

typedef struct LatencyThreadData {
    uint64_t start;       // initial value to write into target
    uint64_t iterations;  // number of iterations to run
    LONG64 *target;       // value to bounce between threads, init with start - 1
    LONG64 *readTarget;   // for read test, memory location to read from (owned by other core)
    //DWORD affinityMask;   // thread affinity mask to set
} LatencyData;

int main(int argc, char *argv[]) {
    int numProcs;
    TimerResult **latencies;
    uint64_t iter = ITERATIONS;
    int offsets = 1;
    TimerResult (*test)(unsigned int, unsigned int, uint64_t) = RunTest;

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
                test = RunOwnedTest;
                fprintf(stderr, "Using separate cache lines for each thread to write to\n");
            }
            else if (strncmp(arg, "offset", 6) == 0) { //todo: Make Case Insensitive (ie _strnicmp on Windows)
                argIdx++;
                offsets = atoi(argv[argIdx]);
                fprintf(stderr, "Offsets: %d\n", offsets);
            }
        }
    }

    bouncyBase = (LONG64*)_aligned_malloc(64 * (offsets + (test == RunOwnedTest)? 1:0), 4096); //
    bouncy = bouncyBase;
    if (bouncy == NULL) {
        fprintf(stderr, "Could not allocate aligned mem\n");
        return 0;
    }
    numProcs = common_threading_get_num_cpu_cores();
    fprintf(stderr, "Number of CPUs: %u\n", numProcs);
    latencies = (TimerResult **) malloc(sizeof(TimerResult*) * offsets); //Allocating array of pointers (one for each offset) to arrays of results
    if (latencies == NULL) {
        fprintf(stderr, "couldn't allocate result array\n");
        return 0;
    }
    memset(latencies, 0, sizeof(TimerResult*) * offsets); //? Zeroing the pointers? Why? TODO
    TimerResult non_result;
    non_result.per_iter_ns = 0;
    for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        bouncy = (LONG64*)((char*)bouncyBase + offsetIdx * 64);
        latencies[offsetIdx] = (TimerResult*)malloc(sizeof(TimerResult) * numProcs * numProcs);
        TimerResult *latenciesPtr = latencies[offsetIdx];
        
        // Run all to all, skipping testing a core against itself ofc
        // technically can skip the other way around (start j = i + 1) but meh
        for (int i = 0;i < numProcs; i++) {
            for (int j = 0;j < numProcs; j++) {
                latenciesPtr[j + i * numProcs] = i == j ? non_result : test(i, j, iter);
            }
        }
    }

      for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        printf("Cache line offset: %d\n", offsetIdx);
        TimerResult* latenciesPtr = latencies[offsetIdx];

        // print thing to copy to excel
        for (int i = 0;i < numProcs; i++) {
            for (int j = 0;j < numProcs; j++) {
                if (j != 0) printf(",");
                if (j == i) printf("x");
                else printf("%f", latenciesPtr[j + i * numProcs].per_iter_ns); //TODO replace with something generic that allows printing more details if we want. (ie, MSR counts)
            }
            printf("\n");
        }

        free(latenciesPtr);
    }

    free(latencies);
    _aligned_free(bouncyBase);
    return 0;
}


/// <summary>
/// Measures latency from one logical processor core to another
/// </summary>
/// <param name="processor1">processor number 1</param>
/// <param name="processor2">processor number 2</param>
/// <param name="iter">Number of iterations</param>
/// <param name="bouncy">aligned mem to bounce around</param>
/// <returns>latency per iteration in ns</returns>
TimerResult RunTest(unsigned int processor1, unsigned int processor2, uint64_t iter) {
    LatencyData lat1, lat2;
    TimerResult latency;

    *bouncy = 0;
    lat1.iterations = iter;
    lat1.start = 1;
    lat1.target = bouncy;
    lat2.iterations = iter;
    lat2.start = 2;
    lat2.target = bouncy;

    latency = TimeThreads(processor1, processor2, iter, lat1, lat2, LatencyTestThread);
    return latency;
}

TimerResult RunOwnedTest(unsigned int processor1, unsigned int processor2, uint64_t iter) {
    LatencyData lat1, lat2;
    LONG64* target1, * target2;
    TimerResult latency;

    // drop them on different cache lines
    //target1 = (LONG64*)_aligned_malloc(128, 64); // TODO: Remove allocation from here and make generic (ie, bouncyBase)
    target1 = bouncy;// this is ok because we allocate one more cache line than neceesary if owned
    target2 = target1 + 8;
    if (target1 == NULL) {
        fprintf(stderr, "Could not allocate aligned mem\n");
    }

    *target1 = 1;
    *target2 = 0;
    lat1.iterations = iter;
    lat1.start = 3;
    lat1.target = target1;
    lat1.readTarget = target2;
    lat2.iterations = iter;
    lat2.start = 2;
    lat2.target = target2;
    lat2.readTarget = target1;

    latency = TimeThreads(processor1, processor2, iter, lat1, lat2, ReadLatencyTestThread);
    //_aligned_free(target1);
    return latency;
}

/// <summary>
/// Runs one thread of the latency test. should be run in pairs
/// Always writes to target
/// </summary>
/// <param name="param">Latency test params</param>
/// <returns>next value that would have been written to shared memory</returns>
int *LatencyTestThread(void *param) {
    LatencyData *latencyData = (LatencyData *)param;
    uint64_t current = latencyData->start;
    while (current <= 2 * latencyData->iterations) {
        if (_InterlockedCompareExchange64(latencyData->target, current, current - 1) == current - 1) { //TODO: replace with some more generic operation, maybe using C's stdatomics
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
int *ReadLatencyTestThread(void *param) {
    LatencyData* latencyData = (LatencyData*)param;
    uint64_t current = latencyData->start;
    //uint64_t startTsc = __rdtsc();
    volatile uint64_t* read_target = latencyData->readTarget; // needs to be volatile otherwise gcc optimizes it out
    volatile uint64_t* write_target = latencyData->target;
    while (current <= 2 * latencyData->iterations) {
        if (*read_target == current - 1) {
            *(write_target) = current;
            current += 2;
            _mm_sfence(); //TODO: replace with some more generic operation, maybe using C's stdatomics
        }
    }

    return current;
}
