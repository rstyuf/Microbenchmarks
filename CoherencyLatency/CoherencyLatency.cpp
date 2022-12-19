#include <stdio.h>
#include <stdint.h>
#include <intrin.h>
#include <windows.h>
#include "common_timer.h"

#define ITERATIONS 10000000;

float RunTest(unsigned int processor1, unsigned int processor2, uint64_t iter);
float RunOwnedTest(unsigned int processor1, unsigned int processor2, uint64_t iter);
DWORD WINAPI LatencyTestThread(LPVOID param);
DWORD WINAPI ReadLatencyTestThread(LPVOID param);

LONG64* bouncyBase;
LONG64* bouncy;

typedef struct LatencyThreadData {
    uint64_t start;       // initial value to write into target
    uint64_t iterations;  // number of iterations to run
    LONG64 *target;       // value to bounce between threads, init with start - 1
    LONG64 *readTarget;   // for read test, memory location to read from (owned by other core)
    DWORD affinityMask;   // thread affinity mask to set
} LatencyData;

int main(int argc, char *argv[]) {
    SYSTEM_INFO sysInfo;
    DWORD numProcs;
    float** latencies;
    uint64_t iter = ITERATIONS;
    int offsets = 1;
    float (*test)(unsigned int, unsigned int, uint64_t) = RunTest;

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

    bouncyBase = (LONG64*)_aligned_malloc(64 * offsets, 4096);
    bouncy = bouncyBase;
    if (bouncy == NULL) {
        fprintf(stderr, "Could not allocate aligned mem\n");
        return 0;
    }

    GetSystemInfo(&sysInfo);
    numProcs = sysInfo.dwNumberOfProcessors;
    fprintf(stderr, "Number of CPUs: %u\n", numProcs);
    latencies = (float **)malloc(sizeof(float*) * offsets);
    if (latencies == NULL) {
        fprintf(stderr, "couldn't allocate result array\n");
        return 0;
    }
	memset(latencies, 0, sizeof(float) * offsets);

    for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        bouncy = (LONG64*)((char*)bouncyBase + offsetIdx * 64);
        latencies[offsetIdx] = (float*)malloc(sizeof(float) * numProcs * numProcs);
        float *latenciesPtr = latencies[offsetIdx];
        
        // Run all to all, skipping testing a core against itself ofc
        // technically can skip the other way around (start j = i + 1) but meh
        for (int i = 0;i < numProcs; i++) {
            for (int j = 0;j < numProcs; j++) {
                latenciesPtr[j + i * numProcs] = i == j ? 0 : test(i, j, iter);
            }
        }
    }

      for (int offsetIdx = 0; offsetIdx < offsets; offsetIdx++) {
        printf("Cache line offset: %d\n", offsetIdx);
        float* latenciesPtr = latencies[offsetIdx];

        // print thing to copy to excel
        for (int i = 0;i < numProcs; i++) {
            for (int j = 0;j < numProcs; j++) {
                if (j != 0) printf(",");
                if (j == i) printf("x");
                else printf("%f", latenciesPtr[j + i * numProcs]);
            }
            printf("\n");
        }

        free(latenciesPtr);
    }

    free(latencies);
    _aligned_free(bouncyBase);
    return 0;
}

float TimeThreads(unsigned int processor1, unsigned int processor2, uint64_t iter, LatencyData lat1, LatencyData lat2, DWORD (*threadFunc)(LPVOID)) {
    TimerStructure timer;
    TimerResult timer_result;
    HANDLE testThreads[2];
    DWORD tid1, tid2;

    testThreads[0] = CreateThread(NULL, 0, threadFunc, &lat1, CREATE_SUSPENDED, &tid1);
    testThreads[1] = CreateThread(NULL, 0, threadFunc, &lat2, CREATE_SUSPENDED, &tid2);

    if (testThreads[0] == NULL || testThreads[1] == NULL) {
        fprintf(stderr, "Failed to create test threads\n");
        return -1;
    }

    SetThreadAffinityMask(testThreads[0], 1ULL << (uint64_t)processor1);
    SetThreadAffinityMask(testThreads[1], 1ULL << (uint64_t)processor2);

    common_timer_start(&timer);
    ResumeThread(testThreads[0]);
    ResumeThread(testThreads[1]);
    WaitForMultipleObjects(2, testThreads, TRUE, INFINITE);
    common_timer_end(&timer, &timer_result, iter);

    fprintf(stderr, "%d to %d: %f ns\n", processor1, processor2, timer_result.per_iter_ns);

    CloseHandle(testThreads[0]);
    CloseHandle(testThreads[1]);

    // each thread does interlocked compare and exchange iterations times. divide by 2 to get overall count of locked ops
    return timer_result.per_iter_ns / 2; //TODO, change to return result and multiply iter by 2 in timer_end func
}

/// <summary>
/// Measures latency from one logical processor core to another
/// </summary>
/// <param name="processor1">processor number 1</param>
/// <param name="processor2">processor number 2</param>
/// <param name="iter">Number of iterations</param>
/// <param name="bouncy">aligned mem to bounce around</param>
/// <returns>latency per iteration in ns</returns>
float RunTest(unsigned int processor1, unsigned int processor2, uint64_t iter) {
    LatencyData lat1, lat2;
    float latency;

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

float RunOwnedTest(unsigned int processor1, unsigned int processor2, uint64_t iter) {
    LatencyData lat1, lat2;
    LONG64* target1, * target2;
    float latency;

    // drop them on different cache lines
    target1 = (LONG64*)_aligned_malloc(128, 64); // TODO: Remove allocation from here and make generic (ie, bouncyBase)
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
    _aligned_free(target1);
    return latency;
}

/// <summary>
/// Runs one thread of the latency test. should be run in pairs
/// Always writes to target
/// </summary>
/// <param name="param">Latency test params</param>
/// <returns>next value that would have been written to shared memory</returns>
DWORD WINAPI LatencyTestThread(LPVOID param) {
    LatencyData *latencyData = (LatencyData *)param;
    uint64_t current = latencyData->start;
    while (current <= 2 * latencyData->iterations) {
        if (_InterlockedCompareExchange64(latencyData->target, current, current - 1) == current - 1) {
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
DWORD WINAPI ReadLatencyTestThread(LPVOID param) {
    LatencyData* latencyData = (LatencyData*)param;
    uint64_t current = latencyData->start;
    uint64_t startTsc = __rdtsc();
    while (current <= 2 * latencyData->iterations) {
        if (*(latencyData->readTarget) == current - 1) {
            *(latencyData->target) = current;
            current += 2;
            _mm_sfence();
        }
    }

    return current;
}
