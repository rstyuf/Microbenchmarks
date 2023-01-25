#ifndef COHERENCY_LATENCY_H
#define COHERENCY_LATENCY_H

#include "../common/common_common.h"
#define COHERENCYLAT_DEFAULT_ITERATIONS (10000000)
typedef TimerResult (*CoherencyLatencyTestType)(unsigned int, unsigned int, uint64_t, Ptr64b*, TimerStructure*) ;

TimerResult RunCoherencyBounceTest(unsigned int processor1, unsigned int processor2, uint64_t iter, Ptr64b* addr, TimerStructure* timer);
TimerResult RunCoherencyOwnedTest(unsigned int processor1, unsigned int processor2, uint64_t iter, Ptr64b* addr, TimerStructure* timer);
int LatencyTestThread(void *param);
int ReadLatencyTestThread(void *param);

typedef struct LatencyThreadData_t {
    uint64_t start;       // initial value to write into target
    uint64_t iterations;  // number of iterations to run
    Ptr64b *target;       // value to bounce between threads, init with start - 1
    Ptr64b *readTarget;   // for read test, memory location to read from (owned by other core)
} LatencyData;

int CoherencyTestMain(int offsets, int iterations, CoherencyLatencyTestType test, TimerStructure *timer);
int CoherencyTestAllocateTestBuffers(int offsets, int additional_cachelines, Ptr64b** BaseAddress);
void CoherencyTestFreeTestBuffers(Ptr64b* BaseAddress);
int CoherencyTestAllocateResultsBuffers(int numProcs, int offsets, TimerResult ***latencies);
void CoherencyTestFreeResultsBuffers(int offsets, TimerResult **latencies);
void CoherencyTestExecute(int numProcs, int offsets, int iterations, Ptr64b* bouncyBase, TimerResult **latencies, CoherencyLatencyTestType test, TimerStructure *timer);
void CoherencyTestPrintResults(int numProcs, int offsets, TimerResult **latencies);

#endif /*COHERENCY_LATENCY_H*/
