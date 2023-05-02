// MemoryBandwidth.c : Version for linux (x86 and ARM)
// Mostly the same as the x86-only VS version, but a bit more manual
#include "../common/common_common.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef __MINGW32__
#include <sys/syscall.h>
#endif

#include <math.h>
//#include <sys/mman.h>
//#include <sys/sysinfo.h>
#include <errno.h>
//#include <numa.h>
/****
 * TODO:
 *  > Fix Instr BW                      -> Done, I think.
 *  > Fix NUMA                          -> Low priority for now
 *  > Fix TestBankConflict              -> Low priority for now
 *  > Add mode choose specific cores    -> Partially Done, no CLI interface and needs testing. May need extension
 *  > Split into modular stucture       -> Done, minus NUMA, TestBankConflict, needs testing
 *  > Implem Missing methods            -> Low Priority for now
 *  > Implem DataLogger                 -> Done, needs testing
 *  > Implem Into TestRunner
 * 
 * **/


#pragma GCC diagnostic ignored "-Wattributes"

int mem_bw_default_test_sizes[39] = { 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 512, 600, 768, 1024, 1536, 2048,
                               3072, 4096, 5120, 6144, 8192, 10240, 12288, 16384, 24567, 32768, 65536, 98304,
                               131072, 262144, 393216, 524288, 1048576, 1572864, 2097152, 3145728 };



#if IS_ISA_X86_64(ENVTYPE)
float scalar_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute((ms_abi));
extern float sse_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float sse_write(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float sse_ntwrite(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float avx512_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float avx512_write(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float avx512_copy(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float avx512_add(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float repmovsb_copy(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float repmovsd_copy(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float repstosb_write(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float repstosd_write(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern uint32_t readbankconflict(uint32_t *arr, uint64_t arr_length, uint64_t spacing, uint64_t iterations) __attribute__((ms_abi));
extern uint32_t readbankconflict128(uint32_t *arr, uint64_t arr_length, uint64_t spacing, uint64_t iterations) __attribute__((ms_abi));
typedef float (*MemBWFunc)(float*, uint64_t, uint64_t, uint64_t start) __attribute__((ms_abi));
#else
float scalar_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start);
typedef float (*MemBWFunc)(float*, uint64_t, uint64_t, uint64_t start);
extern uint32_t readbankconflict(uint32_t *arr, uint64_t arr_length, uint64_t spacing, uint64_t iterations);
#endif

extern float asm_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float asm_write(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float asm_copy(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float asm_cflip(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));
extern float asm_add(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) __attribute__((ms_abi));

#if IS_ISA_AARCH64(ENVTYPE)
extern void flush_icache(void *arr, uint64_t length);
#endif

#if IS_ISA_X86_64(ENVTYPE)
__attribute((ms_abi)) float instr_read(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) {
#else
float instr_read(float *arr, uint64_t arr_length, uint64_t iterations, uint64_t start) { 
#endif
    void (*nopfunc)(uint64_t) __attribute((ms_abi)) = (__attribute((ms_abi)) void(*)(uint64_t))arr;
    for (int iterIdx = 0; iterIdx < iterations; iterIdx++) nopfunc(iterations);
    return 1.1f;
}
typedef struct BandwidthTestThreadData_t {
    uint64_t iterations;
    uint64_t arr_length;
    uint64_t start;
    float* arr;
    float bw; // written to by the thread
    MemBWFunc bw_func;
    //cpu_set_t cpuset; // if numa set, will set affinity
} BandwidthTestThreadData;

void BandwidthTestThreadUnpacker(void *param);
TimerResult MeasureBw(uint64_t sizeKb, uint64_t iterations, uint64_t threads, int shared, int nopBytes, int branchInterval, int coreNode, int memNode, int* specific_cores, MemBWFunc bw_func,TimerStructure *timer);
void FillInstructionArray(uint64_t *nops, uint64_t sizeKb, int nopSize, int branchInterval); 
void TestBankConflicts(int type);
uint64_t GetIterationCount(uint64_t testSize, uint64_t threads, uint64_t gbToTransfer);
void *ReadBandwidthTestThread(void *param);
uint64_t default_gbToTransfer = 512;
int default_branchInterval = 0; 

#define NUMA_STRIPE 1
#define NUMA_SEQ 2
#define NUMA_CROSSNODE 3
#define NUMA_AUTO 4
int default_numa_mode = 0;
void RunMemoryBandwidthAutoThreadsTest(int* mem_bw_test_sizes, int testSizeCount, int thread_max, uint64_t gbToTransfer, int sleepTime, 
                            int shared, int nopBytes, int branchInterval, int core_node, int mem_node, int* specific_cores,  MemBWFunc bw_func, 
                            TimerResult *results_in, TimerStructure* timer, DataLog* dlog);
void RunMemoryBandwidthTest(int* mem_bw_test_sizes, int testSizeCount, int thread_cnt, uint64_t gbToTransfer, int sleepTime, const char* printstring, 
                            int shared, int nopBytes, int branchInterval, int core_node, int mem_node, int* specific_cores,  MemBWFunc bw_func, 
                            TimerResult *results, TimerStructure* timer, DataLog* dlog);

CMISC_CPU_FEATURES _membw_get_default_runfunc(MemBWFunc* bw_func){
 CMISC_CPU_FEATURES cpuf = common_misc_cpu_features_get();
    *bw_func = asm_read;
    // Setting default bw_func
    #if IS_ISA_X86_64(ENVTYPE)
    // if no method is specified, we'll do default. 
    // How do we choose default? We attempt to pick the best one for x86
    // for aarch64 we'll just use NEON because SVE basically doesn't exist
    *bw_func = scalar_read;
    if (COMMON_MISC_CHECK_CPU_FEATURE(cpuf, COMMON_MISC_CPU_FEATURE_SSE)) {
        *bw_func = sse_read;
    }
    if (COMMON_MISC_CHECK_CPU_FEATURE(cpuf, COMMON_MISC_CPU_FEATURE_AVX1)) {
        *bw_func = asm_read;
    }
    if (COMMON_MISC_CHECK_CPU_FEATURE(cpuf, COMMON_MISC_CPU_FEATURE_AVX512_legacytest)) {
        *bw_func = avx512_read;
    }
    #endif /* IS_ISA_X86_64(ENVTYPE)*/
    return cpuf;
}

#if COMPOUND_TEST
// We don't want the main in this case
#else
int main(int argc, char *argv[]) {
    int threads = 1;
    int cpuid_data[4];
    int shared = 1;
    int sleepTime = 0;
    int nopBytes = 0, testBankConflict = 0;
    int testBankConflict128 = 0;
    int singleSize = 0, autothreads = 0;
    int *mem_bw_test_sizes = mem_bw_default_test_sizes;
    int testSizeCount = sizeof(mem_bw_default_test_sizes) / sizeof(int);
    uint64_t gbToTransfer = default_gbToTransfer;
    int branchInterval = default_branchInterval; 
    int numa_mode = default_numa_mode;
    TimerStructure timer;
    DataLog dlog;
    MemBWFunc bw_func = asm_read;
    CMISC_CPU_FEATURES cpuf = _membw_get_default_runfunc(&bw_func);



    for (int argIdx = 1; argIdx < argc; argIdx++) {
        if (*(argv[argIdx]) == '-') {
            char *arg = argv[argIdx] + 1;
            if (strncmp(arg, "threads", 7) == 0) {
                argIdx++;
                threads = atoi(argv[argIdx]);
                fprintf(stderr, "Using %d threads\n", threads);
            } else if (strncmp(arg, "shared", 6) == 0) {
                shared = 1;
                fprintf(stderr, "Using shared array for all threads\n");
            } else if (strncmp(arg, "private", 7) == 0) {
                shared = 0;
                fprintf(stderr, "Using private array for each thread\n");
            } else if (strncmp(arg, "sleep", 5) == 0) {
                argIdx++;
                sleepTime = atoi(argv[argIdx]);
                fprintf(stderr, "Sleeping for %d second between tests\n", sleepTime);
            } else if (strncmp(arg, "branchinterval", 14) == 0) {
                argIdx++;
                branchInterval = atoi(argv[argIdx]);
                fprintf(stderr, "Will add a branch roughly every %d bytes\n", branchInterval * 8);
            } else if (strncmp(arg, "sizekb", 6) == 0) {
                argIdx++;
                singleSize = atoi(argv[argIdx]);
                fprintf(stderr, "Testing %d KB\n", singleSize);
            } else if (strncmp(arg, "data", 4) == 0) {
                argIdx++;
                gbToTransfer = atoi(argv[argIdx]);
                fprintf(stderr, "Base GB to transfer: %lu\n", gbToTransfer);
            }
            else if (strncmp(arg, "autothreads", 11) == 0) {
                argIdx++;
                autothreads = atoi(argv[argIdx]);
                fprintf(stderr, "Testing bw scaling up to %d threads\n", autothreads);
            }
            #ifdef TODO_NOT_YET_IMPLEMENTED
            else if (strncmp(arg, "numa", 4) == 0) {
                argIdx++;
                fprintf(stderr, "Attempting to be NUMA aware\n");
                if (strncmp(argv[argIdx], "crossnode", 4) == 0) {
                    fprintf(stderr, "Testing node to node bandwidth, 1 GB test size\n");
                    numa_mode = NUMA_CROSSNODE;
                    singleSize = 1048576;
                } else if (strncmp(argv[argIdx], "seq", 3) == 0) {
                    fprintf(stderr, "Filling NUMA nodes one by one\n");
                    numa_mode = NUMA_SEQ;
                } else if (strncmp(argv[argIdx], "stripe", 6) == 0) {
                    fprintf(stderr, "Striping threads across NUMA nodes\n");
                    numa_mode = NUMA_STRIPE;
                }
	        }
            #endif /*TODO_NOT_YET_IMPLEMENTED*/
            else if (strncmp(arg, "method", 6) == 0) {
                argIdx++;
                if (strncmp(argv[argIdx], "scalar", 6) == 0) {
                    bw_func = scalar_read;
                    fprintf(stderr, "Using scalar C code\n");
                } else if (strncmp(argv[argIdx], "asm", 3) == 0) {
                    bw_func = asm_read;
                    fprintf(stderr, "Using ASM code (AVX or NEON)\n");
                } else if (strncmp(argv[argIdx], "write", 5) == 0) {
                    bw_func = asm_write;
                    fprintf(stderr, "Using ASM code (AVX or NEON), testing write bw instead of read\n");
                    #if IS_ISA_X86_64(ENVTYPE)
                    if (COMMON_MISC_CHECK_CPU_FEATURE(cpuf, COMMON_MISC_CPU_FEATURE_AVX512_legacytest)) {
                        fprintf(stderr, "Using AVX-512 because that's supported\n");
                        bw_func = avx512_write;
                    }
                    #endif
                } else if (strncmp(argv[argIdx], "copy", 4) == 0) {
                    bw_func = asm_copy;
                    fprintf(stderr, "Using ASM code (AVX or NEON), testing copy bw instead of read\n");
                    #if IS_ISA_X86_64(ENVTYPE)
                    if (COMMON_MISC_CHECK_CPU_FEATURE(cpuf, COMMON_MISC_CPU_FEATURE_AVX512_legacytest)) {
                        fprintf(stderr, "Using AVX-512 because that's supported\n");
                        bw_func = avx512_copy;
                    }
                    #endif
                } else if (strncmp(argv[argIdx], "cflip", 5) == 0) {
                    bw_func = asm_cflip;
                    fprintf(stderr, "Using ASM code (AVX or NEON), flipping order of elements within cacheline\n");
                } else if (strncmp(argv[argIdx], "add", 3) == 0) {
                    bw_func = asm_add;
                    fprintf(stderr, "Using ASM code (AVX or NEON), adding constant to array\n");
                    #if IS_ISA_X86_64(ENVTYPE)
                    if (COMMON_MISC_CHECK_CPU_FEATURE(cpuf, COMMON_MISC_CPU_FEATURE_AVX512_legacytest)) {
                        fprintf(stderr, "Using AVX-512 because that's supported\n");
                        bw_func = avx512_add;
                    }
                    #endif
                }

                else if (strncmp(argv[argIdx], "instr8", 6) == 0) {
                    nopBytes = 8;
                    bw_func = instr_read;
                    fprintf(stderr, "Testing instruction fetch bandwidth with 8 byte instructions.\n");
                } else if (strncmp(argv[argIdx], "instr4", 6) == 0) {
                    nopBytes = 4;
                    bw_func = instr_read;
                    fprintf(stderr, "Testing instruction fetch bandwidth with 4 byte instructions.\n");
                } else if (strncmp(argv[argIdx], "instr2", 6) == 0) {
                    nopBytes = 2;
                    bw_func = instr_read;
                    fprintf(stderr, "Testing instruction fetch bandwith with 2 byte instructions.\n");   
                }
                #if IS_ISA_X86_64(ENVTYPE)
                else if (strncmp(argv[argIdx], "avx512", 6) == 0) {
                    bw_func = avx512_read;
                    fprintf(stderr, "Using ASM code, AVX512\n");
                }
                else if (strncmp(argv[argIdx], "sse_write", 9) == 0) {
                    bw_func = sse_write;
                    fprintf(stderr, "Using SSE to test write bandwidth\n");
                }
                else if (strncmp(argv[argIdx], "sse_ntwrite", 11) == 0) {
                    bw_func = sse_ntwrite;
                    fprintf(stderr, "Using SSE NT writes to test write bandwidth\n");
                } 
                else if (strncmp(argv[argIdx], "sse", 3) == 0) {
                    bw_func = sse_read;
                    fprintf(stderr, "Using ASM code, SSE\n");
                }
                else if (strncmp(argv[argIdx], "avx", 3) == 0) {
                    bw_func = asm_read;
                    fprintf(stderr, "Using ASM code, AVX\n");
                } 
                else if (strncmp(argv[argIdx], "repmovsb", 8) == 0) {
                    bw_func = repmovsb_copy;
                    fprintf(stderr, "Using REP MOVSB to copy\n");
                }
                else if (strncmp(argv[argIdx], "repmovsd", 8) == 0) {
                    bw_func = repmovsd_copy;
                    fprintf(stderr, "Using REP MOVSD to copy\n");
                }
                else if (strncmp(argv[argIdx], "repstosb", 9) == 0) {
                    bw_func = repstosb_write;
                    fprintf(stderr, "Using REP STOSB to write\n");
                } 
                else if (strncmp(argv[argIdx], "repstosd", 9) == 0) {
                    bw_func = repstosd_write;
                    fprintf(stderr, "Using REP STOSD to write\n");
                }  
                else if (strncmp(argv[argIdx], "readbankconflict", 16) == 0) {
                    testBankConflict = 1;
                }
                else if (strncmp(argv[argIdx], "read128bankconflict", 19) == 0) {
                    testBankConflict128 = 1;
                }
                #endif
		
            }
        } else {
            fprintf(stderr, "Expected - parameter\n");
            fprintf(stderr, "Usage: [-threads <thread count>] [-private] [-method <scalar/asm/avx512>] [-sleep <time in seconds>] [-sizekb <single test size>]\n");
        }
    }

/////
///TODO: Split into seperate doer functions and make main into a disabled when compound
    int single_size_sizes_arr[1];
    single_size_sizes_arr[0] = singleSize;
    if (singleSize){
        mem_bw_test_sizes = single_size_sizes_arr;
        testSizeCount = 1;
    }


#ifdef TODO_NOT_YET_IMPLEMENTED
    if (testBankConflict) {
        TestBankConflicts(0);
    } else if (testBankConflict128) {
        TestBankConflicts(1);
    } else 
    #endif /*TODO_NOT_YET_IMPLEMENTED*/
    /*else*/ if (autothreads > 0) {
        RunMemoryBandwidthAutoThreadsTest(mem_bw_test_sizes, testSizeCount, autothreads, gbToTransfer, sleepTime,  shared, nopBytes, branchInterval, 0, 0, NULL,  bw_func, NULL, &timer, &dlog);
    }
    #ifdef TODO_NOT_YET_IMPLEMENTED
     else if (numa_mode == NUMA_CROSSNODE) {
        if (numa_available() == -1) {
	        fprintf(stderr, "NUMA is not available\n");
	        return 0;
	    }

        struct bitmask *nodeBitmask = numa_allocate_cpumask();
        int numaNodeCount = numa_max_node() + 1;
        fprintf(stderr, "System has %d NUMA nodes\n", numaNodeCount);
        float *crossnodeBandwidths = (float *)malloc(sizeof(float) * numaNodeCount * numaNodeCount);
        memset(crossnodeBandwidths, 0, sizeof(float) * numaNodeCount * numaNodeCount);
        for (int cpuNode = 0; cpuNode < numaNodeCount; cpuNode++) {
            numa_node_to_cpus(cpuNode, nodeBitmask);
            int nodeCpuCount = numa_bitmask_weight(nodeBitmask);
            if (nodeCpuCount == 0) {
                fprintf(stderr, "Node %d has no cores\n", cpuNode);
                continue;
            }

            fprintf(stderr, "Node %d has %d cores\n", cpuNode, nodeCpuCount);
            for (int memNode = 0; memNode < numaNodeCount; memNode++) {
                fprintf(stderr, "Testing CPU node %d to mem node %d\n", cpuNode, memNode);
                crossnodeBandwidths[cpuNode * numaNodeCount + memNode] = 
                MeasureBw(singleSize, GetIterationCount(singleSize, nodeCpuCount, gbToTransfer), nodeCpuCount, shared, nopBytes, branchInterval cpuNode, memNode, NULL, bw_func, &timer);
                fprintf(stderr, "CPU node %d <- mem node %d: %f\n", cpuNode, memNode, crossnodeBandwidths[cpuNode * numaNodeCount + memNode]);
            }
        }

        for (int memNode = 0; memNode < numaNodeCount; memNode++) {
            printf(",%d", memNode);
        }

        printf("\n");
        for (int cpuNode = 0; cpuNode < numaNodeCount; cpuNode++) {
            printf("%d", cpuNode);
            for (int memNode = 0; memNode < numaNodeCount; memNode++) {
                printf(",%f", crossnodeBandwidths[cpuNode * numaNodeCount + memNode]);
            }

            printf("\n");
        }

        numa_free_cpumask(nodeBitmask);
    	free(crossnodeBandwidths);
    } 
    #endif /*TODO_NOT_YET_IMPLEMENTED for disabled NUMA logic*/
    else {
        printf("Using %d threads\n", threads);
        RunMemoryBandwidthTest(mem_bw_test_sizes, testSizeCount, threads, gbToTransfer, sleepTime, "%d,%f\n", shared, nopBytes, branchInterval, 0, 0, NULL, bw_func, 
        NULL, &timer, &dlog);
    }

    return 0;
}
#endif /*! COMPOUND_TEST*/
void RunMemoryBandwidthAutoThreadsTest(int* mem_bw_test_sizes, int testSizeCount, int thread_max, uint64_t gbToTransfer, int sleepTime, 
                            int shared, int nopBytes, int branchInterval, int core_node, int mem_node, int* specific_cores,  MemBWFunc bw_func, 
                            TimerResult *results_in, TimerStructure* timer, DataLog* dlog){
    TimerResult *threadResults = (results_in == NULL) ? ( (TimerResult *)malloc(sizeof(TimerResult) * thread_max * testSizeCount)) : results_in;
    #ifndef QUIET_OUTPUT
    printf("Auto threads mode, up to %d threads\n", thread_max);
    #endif
    for (int threadIdx = 1; threadIdx <= thread_max; threadIdx++) {

        int _startidx = (threadIdx - 1) * testSizeCount; // works for both singleSize and multi-size
        char printstr[64] = {0};
        //char* printstr = &(printstr_buf[0]);
        snprintf(printstr, 63, "%d threads, %s", threadIdx, "%d KB total: %f GB/s\n");
        RunMemoryBandwidthTest(mem_bw_test_sizes, testSizeCount, threadIdx, gbToTransfer, sleepTime, (testSizeCount==1? NULL: printstr),
                                shared, nopBytes, branchInterval, 0, 0, NULL, bw_func, 
                                &(threadResults[_startidx]), timer, dlog);

        if (testSizeCount==1) {
            #ifndef QUIET_OUTPUT
            fprintf(stderr, "%d threads: %f GB/s\n", threadIdx, threadResults[threadIdx - 1].result);
            #endif 
        }
    }
    FILE* dlogfd = common_datalogger_log_getfd(dlog);

    if (testSizeCount==1) {
        fprintf(dlogfd,"Threads, BW (GB/s)\n");
        for (int i = 0;i < thread_max; i++) {
            fprintf(dlogfd, "%d,%f\n", i + 1, threadResults[i].result);
        }
    } else {
        fprintf(dlogfd,"Test size down, threads across, value = GB/s\n");
        for (int sizeIdx = 0; sizeIdx < testSizeCount; sizeIdx++) {
            fprintf(dlogfd,"%d", mem_bw_test_sizes[sizeIdx]);
            for (int threadIdx = 1; threadIdx <= thread_max; threadIdx++) {
                fprintf(dlogfd,",%f", threadResults[(threadIdx - 1) * testSizeCount + sizeIdx].result);
            }
            fprintf(dlogfd,"\n");
        }
    }
    if (results_in == NULL)
        free(threadResults);

}


/**
 *        RunMemoryBandwidthTest(mem_bw_test_sizes, testSizeCount, threads, gbToTransfer, sleepTime, "%d,%f\n", shared, nopBytes, branchInterval, 0, 0, NULL, bw_func, 
        TimerResult *threadResults, &timer, &dlog);
 * */

//uint64_t sizeKb, uint64_t iterations, uint64_t threads, int shared, int nopBytes, int branchInterval, int coreNode, int memNode, int* specific_cores, MemBWFunc bw_func, TimerStructure *timer) {
void RunMemoryBandwidthTest(int* mem_bw_test_sizes, int testSizeCount, int thread_cnt, uint64_t gbToTransfer, int sleepTime, const char* printstring, 
                            int shared, int nopBytes, int branchInterval, int core_node, int mem_node, int* specific_cores,  MemBWFunc bw_func, 
                            TimerResult *results, TimerStructure* timer, DataLog* dlog){

    TimerResult default_holder;
    TimerResult *thisresult;
    for (int i = 0; i < testSizeCount; i++) {
        thisresult = (results == NULL)? &default_holder : &(results[i]);
        int currentTestSize = mem_bw_test_sizes[i];
        int iteration_count = GetIterationCount(currentTestSize, thread_cnt, gbToTransfer);
        //fprintf(stderr, "Testing size %d\n", currentTestSize);
        *thisresult = MeasureBw(currentTestSize, iteration_count, thread_cnt, shared, nopBytes, branchInterval, core_node, mem_node, specific_cores, bw_func, timer);
        common_datalogger_log_bandwidth(dlog, *thisresult, " ", currentTestSize, thread_cnt);

        if (printstring != NULL) {
            #ifndef QUIET_OUTPUT
            fprintf(stderr, printstring, currentTestSize, thisresult->result);
            #endif
        }
        if (sleepTime > 0) common_misc_sleep_sec(sleepTime);
    }
}


/// <summary>
/// Given test size in KB, return a good iteration count
/// </summary>
/// <param name="testSize">test size in KB</param>
/// <returns>Iterations per thread</returns>
uint64_t GetIterationCount(uint64_t testSize, uint64_t threads, uint64_t gbToTransfer)
{
    if (testSize > 64) gbToTransfer = 64;
    if (testSize > 512) gbToTransfer = 64;
    if (testSize > 8192) gbToTransfer = 64;
    uint64_t iterations = gbToTransfer * 1024 * 1024 / testSize;
    if (iterations % 2 != 0) iterations += 1;  // must be even

    if (iterations < 8) return 8; // set a minimum to reduce noise
    else return iterations;
}
#ifdef TODO_NOT_YET_IMPLEMENTED
// 0 = scalar, 1 = 128-bit
void TestBankConflicts(int type) {
    struct timeval startTv, endTv;
    time_t time_diff_ms;
    uint32_t *arr;
    uint32_t maxSpacing = 256;
    uint64_t totalLoads = 6e9;

    float *resultArr = malloc((maxSpacing + 1) * sizeof(float));
    int testSize = 4096;
    if (0 != posix_memalign((void **)(&arr), testSize, testSize)) {
        fprintf(stderr, "Could not allocate memory for size %d\n", testSize);
        return;
    }

    for (int spacing = 0; spacing <= maxSpacing; spacing++) {
        *arr = spacing;

        gettimeofday(&startTv, NULL);
        int rc;
        if (type == 0) rc = readbankconflict(arr, testSize, spacing, totalLoads);
        else if (type == 1) rc = readbankconflict128(arr, testSize, spacing, totalLoads);
        gettimeofday(&endTv, NULL);
        time_diff_ms = 1e6 * (endTv.tv_sec - startTv.tv_sec) + (endTv.tv_usec - startTv.tv_usec);
        // want loads per ns
        float loadsPerNs = (float)totalLoads / (time_diff_ms * 1e3);
        fprintf(stderr, "%d KB, %d spacing: %f loads per ns\n", testSize, spacing, loadsPerNs);
        resultArr[spacing] = loadsPerNs;
        if (rc != 0) fprintf(stderr, "asm code returned error\n");
    }

    free(arr);
    arr = NULL;

    for (int spacing = 0; spacing <= maxSpacing; spacing++) printf(",%d", spacing);
    printf("\n");
    for (int spacing = 0; spacing <= maxSpacing; spacing++) {
          printf("%d,%f\n", spacing, resultArr[spacing]);
    }

    free(resultArr);
}


float MeasureInstructionBw(uint64_t sizeKb, uint64_t iterations, int nopSize, int branchInterval) {
#ifdef __x86_64
    char nop2b[8] = { 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90 };
    char nop2b_xor[8] = { 0x31, 0xc0, 0x31, 0xc0, 0x31, 0xc0, 0x31, 0xc0 };
    char nop8b[8] = { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // zen/piledriver optimization manual uses this pattern
    char nop4b[8] = { 0x0F, 0x1F, 0x40, 0x00, 0x0F, 0x1F, 0x40, 0x00 };

    // athlon64 (K8) optimization manual pattern
    char k8_nop4b[8] = { 0x66, 0x66, 0x66, 0x90, 0x66, 0x66, 0x66, 0x90 };
    char nop4b_with_branch[8] = { 0x0F, 0x1F, 0x40, 0x00, 0xEB, 0x00, 0x66, 0x90 };
#endif

#ifdef __aarch64__
    char nop4b[8] = { 0x1F, 0x20, 0x03, 0xD5, 0x1F, 0x20, 0x03, 0xD5 };

    // hack this to deal with graviton 1 / A72
    // nop + mov x0, 0
    char nop8b[9] = { 0x1F, 0x20, 0x03, 0xD5, 0x00, 0x00, 0x80, 0xD2 }; 
    // mov x0, 0 + ldr x0, [sp] 
    char nop8b1[9] = { 0x00, 0x00, 0x80, 0xD2, 0xe0, 0x03, 0x40, 0xf9 }; 
#endif

    struct timeval startTv, endTv;
    struct timezone startTz, endTz;
    float bw = 0;
    uint64_t *nops;
    uint64_t elements = sizeKb * 1024 / 8;
    size_t funcLen = sizeKb * 1024 + 4;   // add 4 bytes to cover for aarch64 ret as well. doesn't hurt for x86

    void (*nopfunc)(uint64_t) __attribute((ms_abi));

    // nops, dec rcx (3 bytes), jump if zero flag set to 32-bit displacement (6 bytes), ret (1 byte)
    //nops = (uint64_t *)malloc(funcLen);
    if (0 != posix_memalign((void **)(&nops), 4096, funcLen)) {
        fprintf(stderr, "Failed to allocate memory for size %lu\n", sizeKb);
        return 0;
    }

    uint64_t *nop8bptr;
    if (nopSize == 8) nop8bptr = (uint64_t *)(nop8b);
    else if (nopSize == 4) nop8bptr = (uint64_t *)(nop4b);
    else if (nopSize == 2) nop8bptr = (uint64_t *)(nop2b_xor);
    else {
        fprintf(stderr, "%d byte instruction length isn't supported :(\n", nopSize);
    }

    for (uint64_t nopIdx = 0; nopIdx < elements; nopIdx++) {
        nops[nopIdx] = *nop8bptr;
#ifdef __x86_64
	uint64_t *nopBranchPtr = (uint64_t *)nop4b_with_branch;
	if (branchInterval > 1 && nopIdx % branchInterval == 0) nops[nopIdx] = *nopBranchPtr;
#endif
#ifdef __aarch64__
	if (nopSize == 8) {
          uint64_t *otherNops = (uint64_t *)nop8b1;
          if (nopIdx & 1) nops[nopIdx] = *otherNops;
	}
#endif
    }

    // ret
    #ifdef __x86_64
    unsigned char *functionEnd = (unsigned char *)(nops + elements);
    functionEnd[0] = 0xC3;
    #endif
    #ifdef __aarch64__
    uint64_t *functionEnd = (uint64_t *)(nops + elements);
    functionEnd[0] = 0XD65F03C0;
    flush_icache((void *)nops, funcLen);
    __builtin___clear_cache(nops, functionEnd);
    #endif

    uint64_t nopfuncPage = (~0xFFF) & (uint64_t)(nops);
    size_t mprotectLen = (0xFFF & (uint64_t)(nops)) + funcLen;
    if (mprotect((void *)nopfuncPage, mprotectLen, PROT_EXEC | PROT_READ | PROT_WRITE) < 0) {
        fprintf(stderr, "mprotect failed, errno %d\n", errno);
        return 0;
    }

    nopfunc = (__attribute((ms_abi)) void(*)(uint64_t))nops;
    gettimeofday(&startTv, &startTz);
    for (int iterIdx = 0; iterIdx < iterations; iterIdx++) nopfunc(iterations);
    gettimeofday(&endTv, &endTz);

    uint64_t time_diff_ms = 1000 * (endTv.tv_sec - startTv.tv_sec) + ((endTv.tv_usec - startTv.tv_usec) / 1000);
    double gbTransferred = (iterations * 8 * elements + 1)  / (double)1e9;
    //fprintf(stderr, "%lf GB transferred in %ld ms\n", gbTransferred, time_diff_ms);
    bw = 1000 * gbTransferred / (double)time_diff_ms;

    free(nops);
    return bw;
}


#endif /*TODO_NOT_YET_IMPLEMENTED instruction BW and bank conflict*/

//#ifdef TODO_NOT_YET_IMPLEMENTED
void FillInstructionArray(uint64_t *nops, uint64_t sizeKb, int nopSize, int branchInterval) {
#if IS_ISA_X86_64(ENVTYPE) 
    char nop2b[8] = { 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90 };
    char nop2b_xor[8] = { 0x31, 0xc0, 0x31, 0xc0, 0x31, 0xc0, 0x31, 0xc0 };
    char nop8b[8] = { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // zen/piledriver optimization manual uses this pattern
    char nop4b[8] = { 0x0F, 0x1F, 0x40, 0x00, 0x0F, 0x1F, 0x40, 0x00 };

    // athlon64 (K8) optimization manual pattern
    char k8_nop4b[8] = { 0x66, 0x66, 0x66, 0x90, 0x66, 0x66, 0x66, 0x90 };
    char nop4b_with_branch[8] = { 0x0F, 0x1F, 0x40, 0x00, 0xEB, 0x00, 0x66, 0x90 };

#elif IS_ISA_AARCH64(ENVTYPE)

    char nop4b[8] = { 0x1F, 0x20, 0x03, 0xD5, 0x1F, 0x20, 0x03, 0xD5 };

    // hack this to deal with graviton 1 / A72
    // nop + mov x0, 0
    char nop8b[9] = { 0x1F, 0x20, 0x03, 0xD5, 0x00, 0x00, 0x80, 0xD2 }; 
    // mov x0, 0 + ldr x0, [sp] 
    char nop8b1[9] = { 0x00, 0x00, 0x80, 0xD2, 0xe0, 0x03, 0x40, 0xf9 }; 
#endif
    
    uint64_t *nop8bptr;
    if (nopSize == 8) nop8bptr = (uint64_t *)(nop8b);
    else if (nopSize == 4) nop8bptr = (uint64_t *)(nop4b);
    else if (nopSize == 2) nop8bptr = (uint64_t *)(nop2b_xor);
    else {
        fprintf(stderr, "%d byte instruction length isn't supported :(\n", nopSize);
    }

    uint64_t elements = sizeKb * 1024 / 8 - 1;
    for (uint64_t nopIdx = 0; nopIdx < elements; nopIdx++) {
        nops[nopIdx] = *nop8bptr;
#if IS_ISA_X86_64(ENVTYPE) 
	uint64_t *nopBranchPtr = (uint64_t *)nop4b_with_branch;
	if (branchInterval > 1 && nopIdx % branchInterval == 0) nops[nopIdx] = *nopBranchPtr;
#elif IS_ISA_AARCH64(ENVTYPE)
	if (nopSize == 8) {
          uint64_t *otherNops = (uint64_t *)nop8b1;
          if (nopIdx & 1) nops[nopIdx] = *otherNops;
	}
#endif
    }

    // ret
#if IS_ISA_X86_64(ENVTYPE) 
    unsigned char *functionEnd = (unsigned char *)(nops + elements);
    functionEnd[0] = 0xC3;
#elif IS_ISA_AARCH64(ENVTYPE)
    uint64_t *functionEnd = (uint64_t *)(nops + elements);
    functionEnd[0] = 0XD65F03C0;
    flush_icache((void *)nops, funcLen);
    __builtin___clear_cache(nops, functionEnd);
#endif

    size_t funcLen = sizeKb * 1024;
    uint64_t nopfuncPage = (~0xFFF) & (uint64_t)(nops);
    size_t mprotectLen = (0xFFF & (uint64_t)(nops)) + funcLen;
    if (!common_mem_special_protect((void *)nopfuncPage, mprotectLen, true, true, true, 0)){ 
        fprintf(stderr,"Protect failed!");
    }
    //#if IS_GCC_POSIX(ENVTYPE)
    //if (mprotect((void *)nopfuncPage, mprotectLen, PROT_EXEC | PROT_READ | PROT_WRITE) < 0) {
    //    fprintf(stderr, "mprotect failed, errno %d\n", errno);
    //}
    //#endif
}
//#else
//void FillInstructionArray(uint64_t *nops, uint64_t sizeKb, int nopSize, int branchInterval){}


//#endif /*TODO_NOT_YET_IMPLEMENTED instruction BW and bank conflict*/

int AllocAndFillArray(float** array, int elements, int thread_id, int nopBytes, int branchInterval, int coreNode, int memNode){

    if (*array == NULL){
        SpecialAllocExtraParams allocparams;
        common_mem_struct_init_SpecialAllocExtraParams(&allocparams);
        allocparams.guarantee_protectable = true;
        if (nopBytes > 0) 
                allocparams.permit_exec = true;

        *array = (float*) common_mem_malloc_special(elements * sizeof(float), 4096, false, &allocparams);

 
        //*array = (float*) common_mem_malloc_aligned(elements * sizeof(float), 4096);
        //if (0 != posix_memalign((void **)(&(threadData[i].arr)), 4096, elements * sizeof(float)))
        if (*array == NULL) {
            fprintf(stderr, "Could not allocate memory!");
            if (thread_id < 0)
                fprintf(stderr, "\n");
            else
                fprintf(stderr, " for thread %ld!\n", thread_id);
            return -1;
        }
    }

    if (nopBytes == 0) {
        int noise_offset = (thread_id < 0)? 0: thread_id;
        for (uint64_t arr_idx = 0; arr_idx < elements; arr_idx++) {  
            (*array)[arr_idx] = arr_idx + noise_offset + 0.5f;
        }
    } else FillInstructionArray((uint64_t *)(*array), elements * sizeof(float) / 1024, nopBytes, branchInterval);
    return 0;
}



// If coreNode and memNode are set, use the specified numa config
// otherwise if numa is set to stripe or seq, respect that
//Maybe add alternative mode where 
TimerResult MeasureBw(uint64_t sizeKb, uint64_t iterations, uint64_t threads, int shared, int nopBytes, int branchInterval, int coreNode, int memNode, int* specific_cores, MemBWFunc bw_func, TimerStructure *timer) {
    TimerResult timer_result;
    
    uint64_t elements = sizeKb * 1024 / sizeof(float);

    if (!shared && sizeKb < threads) {
        fprintf(stderr, "Too many threads for this test size\n");
        timer_result.result = -1;
        return timer_result;
    }

    // make sure this is divisble by 512 bytes, since the unrolled asm loop depends on that
    // it's hard enough to get close to theoretical L1D BW as is, so we don't want additional cmovs or branches
    // in the hot loop
    uint64_t private_elements = ceil((double)sizeKb / (double)threads) * 256;
    //fprintf(stderr, "Actual data: %lu B\n", private_elements * 4 * threads);
    //fprintf(stderr, "Data per thread: %lu B\n", private_elements * 4);

    // make array and fill it with something, if shared
    float* testArr = NULL;
    if (shared){
        if (AllocAndFillArray(&testArr, elements, -1, nopBytes, branchInterval, -1, -1) < 0 
        || (testArr == NULL) ) 
        {
            fprintf(stderr, "Could not allocate memory\n");
            timer_result.result = -1;
            return timer_result;
        }        
    }
    else { //private arrays
        elements = private_elements; // will fill arrays below, per-thread
    }

    //pthread_t* testThreads = (pthread_t*)malloc(threads * sizeof(pthread_t));
    //struct BandwidthTestThreadData* threadData = (struct BandwidthTestThreadData*)malloc(threads * sizeof(struct BandwidthTestThreadData));

    TimeThreadData* testThreads = (TimeThreadData*)malloc(threads * sizeof(TimeThreadData));
    BandwidthTestThreadData* threadData = (BandwidthTestThreadData*)malloc(threads * sizeof(BandwidthTestThreadData));

     #ifdef TODO_NOT_YET_IMPLEMENTED
    // if numa, tell each thread to set an affinity mask
    struct bitmask *nodeBitmask = NULL;
    cpu_set_t cpuset;
    
    if (numa_mode == NUMA_CROSSNODE) {
        nodeBitmask = numa_allocate_cpumask();
	    int nprocs = get_nprocs();
        numa_node_to_cpus(coreNode, nodeBitmask); 
	    CPU_ZERO(&cpuset);

        // provided functions for manipultaing bitmask don't work
        // for (int i = 0; i < nprocs; i++)
        //   if (numa_bitmask_isbitset(nodeBitmask, i)) CPU_SET(i, &cpuset);
        // bitmask has fields:
        // - size = number of bits
        // - maskp = pointer to bitmap
        // cpu_set_t has field __bits. have to assume it's CPU_SETSIZE bits
        // also assume bitmap size is divisible by 8 (byte size)
        memcpy(cpuset.__bits, nodeBitmask->maskp, nodeBitmask->size / 8);
    }
    #endif /*TODO_NOT_YET_IMPLEMENTED*/


    for (uint64_t i = 0; i < threads; i++) {
        if (shared) {
            threadData[i].arr = testArr;
            threadData[i].iterations = iterations;
        }
        else {
            threadData[i].arr = NULL;
            
            #ifdef TODO_NOT_YET_IMPLEMENTED
            int cpuCount = common_threading_get_num_cpu_cores();
            
            if (numa_mode == NUMA_CROSSNODE) {
                threadData[i].arr = numa_alloc_onnode(elements * sizeof(float), memNode);
                threadData[i].cpuset = cpuset;
            } else if (numa_mode) {
                // Figure out which nodes actually have CPUs and memory
                //int numaNodeCount = numa_max_node() + 1;
                int numaNodeCount = 4;   // for knl. geez
                if (numa_mode == NUMA_SEQ) {
                // unimplemented
                fprintf(stderr, "sequential numa node fill not implemented yet\n");
                } else if (numa_mode == NUMA_STRIPE) {
                    memNode = i % numaNodeCount;
                    coreNode = memNode;
                }
               
                
                for(int cpuIdx = 0; cpuIdx < get_nprocs(); cpuIdx++) {
                    CPU_ZERO(&(threadData[i].cpuset));
                    if(CPU_ISSET(i, &(threadData[i].cpuset))) {
                        fprintf(stderr, "bitmask not cleared\n");
                    }
                }

                threadData[i].arr = numa_alloc_onnode(elements * sizeof(float), memNode);

                for(int cpuIdx = 0; cpuIdx < get_nprocs(); cpuIdx++) {
                    CPU_ZERO(&(threadData[i].cpuset));
                    if(CPU_ISSET(i, &(threadData[i].cpuset))) {
                        fprintf(stderr, "bitmask not cleared\n");
                    }
                }

                // cpu node affinity has to be set for each thread
                nodeBitmask = numa_allocate_cpumask();
                numa_node_to_cpus(coreNode, nodeBitmask); 
                CPU_ZERO(&(threadData[i].cpuset));
                //fprintf(stderr, "Node %d has CPUs:", coreNode);
                for (int cpuIdx = 0; cpuIdx < cpuCount; cpuIdx++) { 
                    if (numa_bitmask_isbitset(nodeBitmask, cpuIdx))  {
                        CPU_SET(cpuIdx, &(threadData[i].cpuset)); 
                        //fprintf(stderr, " %d", cpuIdx);
                    }
                }

                //fprintf(stderr, "\n\n");
            } 
            #endif /*TODO_NOT_YET_IMPLEMENTED*/

            if (AllocAndFillArray(&(threadData[i].arr), elements, i, nopBytes, branchInterval, -1, -1) < 0 
                || threadData[i].arr == NULL) 
            {
                fprintf(stderr, "Could not allocate memory for thread %ld\n", i);
                timer_result.result = -1;
                return timer_result;
            }
            threadData[i].iterations = iterations * threads;
        }
         

        threadData[i].arr_length = elements;
        threadData[i].bw = 0;
        threadData[i].start = 0;
        threadData[i].bw_func = bw_func;
        //if (elements > 8192 * 1024) threadData[i].start = 4096 * i; // must be multiple of 128 because of unrolling
        //int pthreadRc = pthread_create(testThreads + i, NULL, ReadBandwidthTestThread, (void *)(threadData + i));
        testThreads[i].threadFunc = (void*) BandwidthTestThreadUnpacker;
        testThreads[i].pArg = &(threadData[i]);
        if (specific_cores == NULL)
           testThreads[i].processorIdx = -1;
        else {
            testThreads[i].processorIdx = specific_cores[i];
        }
    }
    //////////////////////
    timer_result = TimeThreads(testThreads, threads, timer);
    /*
    gettimeofday(&startTv, &startTz);
    for (uint64_t i = 0; i < threads; i++) pthread_create(testThreads + i, NULL, ReadBandwidthTestThread, (void *)(threadData + i));
    for (uint64_t i = 0; i < threads; i++) pthread_join(testThreads[i], NULL);
    gettimeofday(&endTv, &endTz);
    uint64_t time_diff_ms = 1000 * (endTv.tv_sec - startTv.tv_sec) + ((endTv.tv_usec - startTv.tv_usec) / 1000);
    */
    double gbTransferred = (iterations * sizeof(float) * elements * threads) ;
    //bw = 1000 * gbTransferred / (double)time_diff_ms;
    //if (!shared) bw = bw * threads; // iteration count is divided by thread count if in thread private mode
    //printf("%f GB, %lu ms\n", gbTransferred, time_diff_ms);
    //printf("DBG: gbTransf %f b, iter  %d, elem %d, threads %d . " ,gbTransferred, iterations, elements, threads);
    gbTransferred = gbTransferred * (double)1e-9;
    //printf("DBG: gbTransf %f gb --  ",gbTransferred );
    gbTransferred = gbTransferred * ((!shared)? threads:1);
    //printf("DBG: gbTransf %f gb, time %d ms, <> %f \n ",gbTransferred, timer_result.time_dif_ms,  1000 * gbTransferred / ((double)(timer_result.time_dif_ms)));
     //results->result = ; 

    common_timer_result_process_bw(&timer_result, gbTransferred);
    //printf("DBG: RESULT: %f RES  ", timer_result.result);

    //common_timer_result_process_bw(&timer_result, gbTransferred * ((!shared)? threads:1));
    ////    results->result =  1000 * transferred / ((double)(results->time_dif_ms)); 
    #ifdef TODO_NOT_YET_IMPLEMENTED
    if (numa_mode) numa_free_cpumask(nodeBitmask);
    #endif /*TODO_NOT_YET_IMPLEMENTED*/
    free(testThreads);
    testThreads = NULL;
    if (shared) {
        common_mem_special_free((Ptr64b*) testArr);// covers test array in shared mode
        testArr = NULL;
    }
    else {
        for (uint64_t i = 0; i < threads; i++) {
         #ifdef TODO_NOT_YET_IMPLEMENTED
            if (numa_mode) numa_free(threadData[i].arr, elements * sizeof(float));
                else 
         #endif /*TODO_NOT_YET_IMPLEMENTED*/
                common_mem_special_free((Ptr64b*) threadData[i].arr);
                threadData[i].arr = NULL;
            }
    }

    free(threadData);
    threadData = NULL;
    return timer_result;
}

#ifdef __x86_64
__attribute((ms_abi)) float scalar_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) {
#else
float scalar_read(float* arr, uint64_t arr_length, uint64_t iterations, uint64_t start) {
#endif
    float sum = 0;
    if (start + 16 >= arr_length) return 0;

    uint64_t iter_idx = 0, i = start;
    float s1 = 0, s2 = 1, s3 = 0, s4 = 1, s5 = 0, s6 = 1, s7 = 0, s8 = 1;
    while (iter_idx < iterations) {
        s1 += arr[i];
        s2 *= arr[i + 1];
        s3 += arr[i + 2];
        s4 *= arr[i + 3];
        s5 += arr[i + 4];
        s6 *= arr[i + 5];
        s7 += arr[i + 6];
        s8 *= arr[i + 7];
        i += 8;
        if (i + 7 >= arr_length) i = 0;
        if (i == start) iter_idx++;
    }

    sum += s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8;

    return sum;
}
/*
void *ReadBandwidthTestThread(void *param) {
    BandwidthTestThreadData* bwTestData = (BandwidthTestThreadData*)param;
    if (numa_mode) {
        int affinity_rc = sched_setaffinity(gettid(), sizeof(cpu_set_t), &(bwTestData->cpuset));
        if (affinity_rc != 0) {
            fprintf(stderr, "wtf set affinity failed: %s\n",strerror(errno));
        }
    }
    float sum = bw_func(bwTestData->arr, bwTestData->arr_length, bwTestData->iterations, bwTestData->start);
    if (sum == 0) printf("woohoo\n");
    pthread_exit(NULL);
}*/

void BandwidthTestThreadUnpacker(void *param) {
    BandwidthTestThreadData* bwTestData = (BandwidthTestThreadData*)param;
    float sum = bwTestData->bw_func(bwTestData->arr, bwTestData->arr_length, bwTestData->iterations, bwTestData->start);
    if (sum == 0) printf(" ");
    return;
}
