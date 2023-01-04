#include "common_common.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "common_timer.h"
#include "common_threading.h"
#include "common_memory.h"

// #include <sys/time.h>
// #include <unistd.h>



// #ifdef NUMA
// #include <numa.h>
// #include <numaif.h>
// #include <sys/sysinfo.h>
// #endif
// #include <errno.h>
// #include <sched.h>

// TODO: possibly get this programatically
#define PAGE_SIZE 4096
#define CACHELINE_SIZE 64
int default_test_sizes[] = { 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 600, 768, 1024, 1536, 2048,
                               3072, 4096, 5120, 6144, 8192, 10240, 12288, 16384, 24567, 32768, 65536, 98304,
                               131072, 262144, 393216, 524288, 1048576 }; //2097152 };

#ifdef __x86_64
extern void preplatencyarr(uint64_t *arr, uint64_t len) __attribute__((ms_abi));
extern uint32_t latencytest(uint64_t iterations, uint64_t *arr) __attribute((ms_abi));
extern void stlftest(uint64_t iterations, char *arr) __attribute((ms_abi));
extern void matchedstlftest(uint64_t iterations, char *arr) __attribute((ms_abi));
extern void stlftest32(uint64_t iterations, char *arr) __attribute((ms_abi));
extern void stlftest128(uint64_t iterations, char *arr) __attribute((ms_abi));
void (*stlfFunc)(uint64_t, char *) __attribute__((ms_abi)) = stlftest;
#elif __i686
extern void preplatencyarr(uint32_t *arr, uint32_t len) __attribute__((fastcall));
extern uint32_t latencytest(uint32_t iterations, uint32_t *arr) __attribute((fastcall));
extern void stlftest(uint32_t iterations, char *arr) __attribute((fastcall));
extern void matchedstlftest(uint32_t iterations, char *arr) __attribute((fastcall));
void (*stlfFunc)(uint32_t, char *) __attribute__((fastcall)) = stlftest;
#define BITS_32
#elif __aarch64__
extern void preplatencyarr(uint64_t *arr, uint64_t len);
extern uint32_t late2ncytest(uint64_t iterations, uint64_t *arr);
extern void matchedstlftest(uint64_t iterations, char *arr);
extern void stlftest(uint64_t iterations, char *arr);
extern void stlftest32(uint64_t iterations, char *arr);
extern void stlftest128(uint64_t iterations, char *arr);
void (*stlfFunc)(uint64_t, char *) = stlftest;
#else
#define UNKNOWN_ARCH 1
extern uint32_t latencytest(uint64_t iterations, uint64_t *arr);
void (*stlfFunc)(uint64_t, char *) = NULL;
#endif

TimerResult RunTest(uint32_t size_kb, uint32_t iterations, uint32_t *preallocatedArr);
TimerResult RunAsmTest(uint32_t size_kb, uint32_t iterations, uint32_t *preallocatedArr);
TimerResult RunTlbTest(uint32_t size_kb, uint32_t iterations, uint32_t *preallocatedArr);
TimerResult RunMlpTest(uint32_t size_kb, uint32_t iterations, uint32_t parallelism);

void MlpTestMain(int mlpTestVal, int* test_sizes, uint32_t testSizeCount);
void StlfTestMain(uint32_t iterations, int mode, int pageEnd, int loadDistance);
void NumaTestMain(uint32_t *preallocatedArr, uint32_t testSize);


TimerResult (*testFunc)(uint32_t, uint32_t, uint32_t *) = RunTest;

uint32_t ITERATIONS = 100000000;

int main(int argc, char* argv[]) {
    uint32_t maxTestSizeMb = 0;
    uint32_t singleSize = 0;
    uint32_t testSizeCount = sizeof(default_test_sizes) / sizeof(int);
    int* test_sizes = default_test_sizes;
    int mlpTest = 0;  // if > 0, run MLP test with (value) levels of parallelism max
    int stlf = 0, hugePages = 0;
    int stlfPageEnd = 0, numa = 0, stlfLoadDistance = 0;
    uint32_t *hugePagesArr = NULL;
    for (int argIdx = 1; argIdx < argc; argIdx++) {
        if (*(argv[argIdx]) == '-') {
            char *arg = argv[argIdx] + 1;
            if (strncmp(arg, "test", 4) == 0) {
                argIdx++;
                char *testType = argv[argIdx];

		        if (strncmp(testType, "c", 1) == 0) {
                    testFunc = RunTest;
                    fprintf(stderr, "Using simple C test\n");
                } else if (strncmp(testType, "tlb", 3) == 0) {
                    testFunc = RunTlbTest;
                    fprintf(stderr, "Testing TLB with one element accessed per 4K page\n");
                } else if (strncmp(testType, "mlp", 3) == 0) {
                    mlpTest = 32;
                    fprintf(stderr, "Running memory parallelism test\n");
                }
                #ifndef UNKNOWN_ARCH
                else if (strncmp(testType, "asm", 3) == 0) {
                    testFunc = RunAsmTest;
                    fprintf(stderr, "Using ASM (simple address) test\n");
                } else if (strncmp(testType, "stlf", 4) == 0) {
                    stlf = 1;
                    fprintf(stderr, "Running store to load forwarding test\n");
                } else if (strncmp(testType, "matched_stlf", 4) == 0) {
                    stlf = 1;
                    stlfFunc = matchedstlftest;
                    fprintf(stderr, "Running store to load forwarding test, with matched load/store sizes\n");
                } else if (strncmp(testType, "128_stlf", 4) == 0) {
                    stlf = 1;
                    stlfFunc = stlftest128;
                    fprintf(stderr, "Running store to load forwarding test, with 128-bit store, 64-bit load\n");
                }
		#ifndef BITS_32
                else if (strncmp(testType, "dword_stlf", 9) == 0) {
                    stlf = 2;
                    stlfFunc = stlftest32;
                    fprintf(stderr, "Running store to load forwarding test, with 32-bit stores\n");
                }
		#endif
		#endif  // end UNKNOWN_ARCH
                else {
                    fprintf(stderr, "Unrecognized test type: %s\n", testType);
                    fprintf(stderr, "Valid test types: c, tlb, mlp");
		        #ifndef UNKNOWN_ARCH
		            fprintf(stderr, ", asm, stlf, matched_stlf, 128_stlf");
		        #ifndef BITS_32
		            fprintf(stderr, ", dword_stlf");
		        #endif /*BITS_32*/
                #endif /*UNKNOWN_ARCH*/
                    fprintf(stderr, "\n");
                 }
            }
            else if (strncmp(arg, "maxsizemb", 9) == 0) {
                argIdx++;
                maxTestSizeMb = atoi(argv[argIdx]);
                fprintf(stderr, "Will not exceed %u MB\n", maxTestSizeMb);
            }
            else if (strncmp(arg, "iter", 4) == 0) {
                argIdx++;
                ITERATIONS = atoi(argv[argIdx]);
                fprintf(stderr, "Base iterations: %u\n", ITERATIONS);
            } 
            else if (strncmp(arg, "stlf_page_end", 13) == 0) {
                    argIdx++;
                    stlfPageEnd = atoi(argv[argIdx]);
                    fprintf(stderr, "Store to load forwarding test will be pushed to end of %d byte page\n", stlfPageEnd);
            }
            else if (strncmp(arg, "stlf_load_offset", 16) == 0) {
                    argIdx++;
                    stlfLoadDistance = atoi(argv[argIdx]);
                    fprintf(stderr, "Loads will be offset by %d bytes\n", stlfLoadDistance);
            }
            //#ifndef __MINGW32__
            else if (strncmp(arg, "hugepages", 9) == 0) {
	              hugePages = 1;
	              fprintf(stderr, "If applicable, will use huge pages. Will allocate max memory at start, make sure system has enough memory.\n");
	        } 
            else if (strncmp(arg, "sizekb", 6) == 0) {
                argIdx++;
                singleSize = atoi(argv[argIdx]);
                fprintf(stderr, "Testing %u KB only\n", singleSize);
            }
            //#endif /*__MINGW32__*/
            else if (strncmp(arg, "numa", 4) == 0) {
            #ifdef NUMA
    	        numa = 1;
                if (singleSize == 0){
		            singleSize = 1048576;
                }
		        fprintf(stderr, "Testing node to node latency. If test size is not set, it will be 1 GB\n");
            #else
            	fprintf(stderr, "NUMA is unsupported in this version\n");
            #endif /*NUMA*/
	        }
	        else {
                fprintf(stderr, "Unrecognized option: %s\n", arg);
            }
        }
    }

    if (argc == 1) {
        fprintf(stderr, "Usage: [-test <c/asm/tlb/mlp>] [-maxsizemb <max test size in MB>] [-iter <base iterations, default 100000000]\n");
    }
    // Call Generic Runner

    if (hugePages) {
        size_t maxTestSizeKb = singleSize ? singleSize : test_sizes[testSizeCount - 1];
        if (maxTestSizeMb){
            maxTestSizeKb = maxTestSizeKb > maxTestSizeMb*1024 ? maxTestSizeMb*1024 : maxTestSizeKb;
            singleSize = (singleSize && singleSize > maxTestSizeMb*1024) ? maxTestSizeMb*1024 : singleSize;
        }
        hugePagesArr = (uint32_t* ) common_mem_malloc_special(maxTestSizeKb*1024, 0, true, COMMON_MEM_ALLOC_NUMA_DISABLED);
        if (hugePagesArr == NULL) return -1;
    }

    if (mlpTest) {
        MlpTestMain(mlpTest, test_sizes, testSizeCount);
    } else if (stlf) {
        StlfTestMain(ITERATIONS, stlf, stlfPageEnd, stlfLoadDistance);
    } 
//#ifdef NUMA
    else if (numa) {
        NumaTestMain(hugePagesArr, singleSize);
    }
//#endif
    else {
        if (singleSize == 0) {
        printf("Region,Latency (ns)\n");
            for (int i = 0; i < testSizeCount; i++) {
                if ((maxTestSizeMb == 0) || (test_sizes[i] <= maxTestSizeMb * 1024))
                    printf("%d,%f\n", test_sizes[i], testFunc(test_sizes[i], ITERATIONS, hugePagesArr).per_iter_ns);
                else {
                    fprintf(stderr, "Test size %u KB exceeds max test size of %u KB\n", test_sizes[i], maxTestSizeMb * 1024);
                    break;
                }
            }
        } else {
            printf("%d,%f\n", singleSize, testFunc(singleSize, ITERATIONS, hugePagesArr).per_iter_ns);
        }
    }

    return 0;
}


void MlpTestMain(int mlpTestVal, int* test_sizes, uint32_t testSizeCount){
    // allocate arr to hold results
    TimerResult *results = (TimerResult*)malloc(testSizeCount * mlpTestVal * sizeof(TimerResult));
    for (int size_idx = 0; size_idx < testSizeCount; size_idx++) {
        for (int parallelism = 0; parallelism < mlpTestVal; parallelism++) {
            results[size_idx * mlpTestVal + parallelism] = RunMlpTest(test_sizes[size_idx], ITERATIONS, parallelism + 1);
            printf("%d KB, %dx parallelism, %f MB/s\n", test_sizes[size_idx], parallelism + 1, results[size_idx * mlpTestVal + parallelism].bw_in_MB);
        }
    }

    for (int size_idx = 0; size_idx < testSizeCount; size_idx++) {
        printf(",%d", test_sizes[size_idx]);
    }

    printf("\n");

    for (int parallelism = 0; parallelism < mlpTestVal; parallelism++) {
        printf("%d", parallelism + 1);
        for (int size_idx = 0; size_idx < test_sizes[size_idx]; size_idx++) {
            printf(",%f", results[size_idx * mlpTestVal + parallelism].bw_in_MB);
        }
        printf("\n");
    }

    free(results);
}

void NumaTestMain(uint32_t *preallocatedArr, uint32_t testSize){
#ifdef NUMA
    int numaNodeCount = common_numa_check_available_node_count();
    if (numaNodeCount < 0) {
        return -1;
    }
    uint32_t *arr = preallocatedArr;


    TimerResult *crossnodeLatencies = (TimerResult *)malloc(sizeof(TimerResult) * numaNodeCount * numaNodeCount);
    memset(crossnodeLatencies, 0, sizeof(TimerResult) * numaNodeCount * numaNodeCount);

    for (int cpuNode = 0; cpuNode < numaNodeCount; cpuNode++) {
        int ret = common_numa_set_cpu_affinity_to_node(cpuNode);
        if (ret == -2) continue;
        else if (ret == -1) return -1;
        for (int memNode = 0; memNode < numaNodeCount; memNode++) {
            if (arr == NULL){
                arr = (uint32_t*) common_mem_malloc_special(testSize, 0, false, memNode);
                if (arr == NULL) return -1;
            } else {
                common_mem_numa_move_mem_node(arr, memNode);
            }      
            TimerResult latency = testFunc(testSize, ITERATIONS, arr);
            crossnodeLatencies[cpuNode * numaNodeCount + memNode] = latency;
            fprintf(stderr, "CPU node %d -> mem node %d: %f ns\n", cpuNode, memNode, latency);
            if (!preallocatedArr) common_mem_special_free(arr);
        }
    }

    for (int memNode = 0; memNode < numaNodeCount; memNode++) {
        printf(",%d", memNode);
    }

    printf("\n");
    for (int cpuNode = 0; cpuNode < numaNodeCount; cpuNode++) {
        printf("%d", cpuNode);
        for (int memNode = 0; memNode < numaNodeCount; memNode++) {
            printf(",%f", crossnodeLatencies[cpuNode * numaNodeCount + memNode].per_iter_ns);
        }

        printf("\n");
    }

    free(crossnodeLatencies);
#endif /*NUMA*/
    fprintf(stderr, "NUMA not supported\n");
}

/// <summary>
/// Heuristic to make sure test runs for enough time but not too long
/// </summary>
/// <param name="size_kb">Region size</param>
/// <param name="iterations">base iterations</param>
/// <returns>scaled iterations</returns>
uint64_t scale_iterations(uint32_t size_kb, uint32_t iterations) {
    return 10 * iterations / pow(size_kb, 1.0 / 4.0);
}

// Fills an array using Sattolo's algo
void FillPatternArr(uint32_t *pattern_arr, uint32_t list_size, uint32_t byte_increment) {
    uint32_t increment = byte_increment / sizeof(uint32_t);
    uint32_t element_count = list_size / increment;
    for (int i = 0; i < element_count; i++) {
        pattern_arr[i * increment] = i * increment;
    }

    int iter = element_count;
    while (iter > 1) {
        iter -= 1;
        int j = iter - 1 == 0 ? 0 : rand() % (iter - 1);
        uint32_t tmp = pattern_arr[iter * increment];
        pattern_arr[iter * increment] = pattern_arr[j * increment];
        pattern_arr[j * increment] = tmp;
    }
}

void FillPatternArr64(uint64_t *pattern_arr, uint64_t list_size, uint64_t byte_increment) {
    uint32_t increment = byte_increment / sizeof(uint64_t);
    uint32_t element_count = list_size / increment;
    for (int i = 0; i < element_count; i++) {
        pattern_arr[i * increment] = i * increment;
    }

    int iter = element_count;
    while (iter > 1) {
        iter -= 1;
        int j = iter - 1 == 0 ? 0 : rand() % (iter - 1);
        uint64_t tmp = pattern_arr[iter * increment];
        pattern_arr[iter * increment] = pattern_arr[j * increment];
        pattern_arr[j * increment] = tmp;
    }
}

TimerResult RunTest(uint32_t size_kb, uint32_t iterations, uint32_t *preallocatedArr) {
    TimerStructure timer;
    TimerResult timer_result;
    uint32_t list_size = size_kb * 1024 / 4;
    uint32_t sum = 0, current;

    // Fill list to create random access pattern
    int *A;
    if (preallocatedArr == NULL) {
        A = (int*)malloc(sizeof(int) * list_size);
        if (!A) {
            fprintf(stderr, "Failed to allocate memory for %u KB test\n", size_kb);
            timer_result.per_iter_ns = -1;
            return timer_result;
        }
    } else {
        A = preallocatedArr;
    }

    FillPatternArr(A, list_size, CACHELINE_SIZE);

    uint32_t scaled_iterations = scale_iterations(size_kb, iterations);

    // Run test
    common_timer_start(&timer);
    current = A[0];
    for (int i = 0; i < scaled_iterations; i++) {
        current = A[current];
        sum += current;
    }
    common_timer_end(&timer, &timer_result, scaled_iterations);
    if (preallocatedArr == NULL) free(A);

    if (sum == 0) printf("sum == 0 (?)\n");
    return timer_result;
}

// Tests memory level parallelism. Returns achieved BW in MB/s using specified number of
// independent pointer chasing chains
TimerResult RunMlpTest(uint32_t size_kb, uint32_t iterations, uint32_t parallelism) {
    TimerStructure timer;
    TimerResult timer_result;
    uint32_t list_size = size_kb * 1024 / 4;
    uint32_t sum = 0, current;

    if (parallelism < 1) {
            fprintf(stderr, "No use in MLP test with no parallelism\n");
            timer_result.per_iter_ns = 0;
            return timer_result;
        }

    // Fill list to create random access pattern, and hold temporary data
    int* A = (int*)malloc(sizeof(int) * list_size);
    int *offsets = (int*)malloc(sizeof(int) * parallelism);
    if (!A || !offsets) {
        fprintf(stderr, "Failed to allocate memory for %u KB test\n", size_kb);
        timer_result.per_iter_ns = -1;
        return timer_result;
    }

    FillPatternArr(A, list_size, CACHELINE_SIZE);
    for (int i = 0; i < parallelism; i++) offsets[i] = i * (CACHELINE_SIZE / sizeof(int));
    uint32_t scaled_iterations = scale_iterations(size_kb, iterations) / parallelism;

    // Run test
    common_timer_start(&timer);
    for (int i = 0; i < scaled_iterations; i++) {
        for (int j = 0; j < parallelism; j++)
        {
            offsets[j] = A[offsets[j]];
        }
    }
    common_timer_end_bw(&timer, &timer_result, scaled_iterations, (parallelism * sizeof(int)));
    
    sum = 0;
    for (int i = 0; i < parallelism; i++) sum += offsets[i];
    if (sum == 0) printf("sum == 0 (?)\n");

    free(A);
    free (offsets);
    return timer_result;
}


#ifndef UNKNOWN_ARCH
TimerResult RunAsmTest(uint32_t size_kb, uint32_t iterations, uint32_t *preallocatedArr) {
    TimerStructure timer;
    TimerResult timer_result;
    uint64_t list_size = size_kb * 1024 / POINTER_SIZE; // using 32-bit pointers
    uint32_t sum = 0, current;

    // Fill list to create random access pattern
    PtrSysb *A;
    if (preallocatedArr == NULL) {
        A = (PtrSysb *)malloc(POINTER_SIZE * list_size);
        if (!A) {
            fprintf(stderr, "Failed to allocate memory for %lu KB test\n", size_kb);
            timer_result.per_iter_ns = -1;
            return timer_result;
	      }
    } else {
        A = (PtrSysb *)preallocatedArr;
    }

    memset(A, 0, POINTER_SIZE * list_size);

#ifdef BITS_32
    FillPatternArr(A, list_size, CACHELINE_SIZE);
#else
    FillPatternArr64(A, list_size, CACHELINE_SIZE);
#endif

    preplatencyarr(A, list_size);

    uint32_t scaled_iterations = scale_iterations(size_kb, iterations);

    // Run test
    common_timer_start(&timer);
    sum = latencytest(scaled_iterations, A);
    common_timer_end(&timer, &timer_result, scaled_iterations);
    if (preallocatedArr == NULL) free(A);

    if (sum == 0) printf("sum == 0 (?)\n");
    return timer_result;
}
#endif

// Tries to isolate virtual to physical address translation latency by accessing
// one element per page, and checking latency difference between that and hitting the same amount of "hot"
// cachelines using a normal latency test.. 4 KB pages are assumed.
TimerResult RunTlbTest(uint32_t size_kb, uint32_t iterations, uint32_t *preallocatedArr) {
    TimerStructure timer;
    TimerResult timer_result;
    uint32_t element_count = size_kb / 4;
    uint32_t list_size = size_kb * 1024 / 4;
    uint32_t sum = 0, current;

    if (element_count == 0) element_count = 1;

    //fprintf(stderr, "Element count for size %u: %u\n", size_kb, element_count);

    // create access pattern first, then fill it into the test array spaced by page size
    uint32_t *pattern_arr = (uint32_t*)malloc(sizeof(uint32_t) * element_count);
    if (!pattern_arr) {
        fprintf(stderr, "Failed to allocate memory for %u KB test (offset array)\n", size_kb);
        timer_result.per_iter_ns = -1;
        return timer_result;
    }

    for (int i = 0; i < element_count; i++) {
        pattern_arr[i] = i;
    }

    int iter = element_count;
    while (iter > 1) {
        iter -= 1;
        int j = iter - 1 == 0 ? 0 : rand() % (iter - 1);
        uint32_t tmp = pattern_arr[iter];
        pattern_arr[iter] = pattern_arr[j];
        pattern_arr[j] = tmp;
    }

    // translate offsets and fill the test array
    // [offset-------page-------][offset-----page------....etc
    uint32_t *A;
    if (preallocatedArr == NULL) {
        A = (uint32_t *)malloc(sizeof(uint32_t) * list_size);
        if (!A) {
            fprintf(stderr, "Failed to allocate memory for %u KB test (pointer array)\n", size_kb);
        }
    } else {
        A = preallocatedArr;
    }

    memset(A, INT_MAX, list_size); // catch any bad accesses immediately
    int pageIncrement = PAGE_SIZE / sizeof(uint32_t);
    for (int i = 0;i < element_count; i++) {
        // offset each by i cachelines to avoid conflict misses. If we just use the first cacheline
        // in each page, the index bits for every VIPT access will be the same and we'll run into L1D misses
        // faster than we would like
        int idx = i * pageIncrement + ((i * 16) & (pageIncrement - 1));
        int target_idx = pattern_arr[i] * pageIncrement + ((pattern_arr[i] * 16) & (pageIncrement - 1));
        A[idx] = target_idx;
    }

    free(pattern_arr);  // don't need this anymore

    uint32_t scaled_iterations = scale_iterations(size_kb, iterations);

    // Run test
    common_timer_start(&timer);
    current = A[0];
    for (int i = 0; i < scaled_iterations; i++) {
        current = A[current];
        sum += current;
        //if (size_kb == 48) fprintf(stderr, "idx: %u\n", current);
    }
    common_timer_end(&timer, &timer_result, scaled_iterations);
    if (preallocatedArr == NULL) free(A);

    if (element_count > 1 && sum == 0) printf("sum == 0 (?)\n");

    // Get a reference timing for the size, to isolate TLB latency from cache latency
    uint32_t memoryUsedKb = (element_count * CACHELINE_SIZE) / 1024;
    if (memoryUsedKb == 0) memoryUsedKb = 1;
    TimerResult cacheLatency = RunTest(memoryUsedKb, iterations, preallocatedArr);

    //fprintf(stderr, "Memory used - %u KB, latency: %f, ref latency: %f\n", memoryUsedKb, latency, cacheLatency);
    return common_timer_result_difference(timer_result, cacheLatency);
}

// Run store to load forwarding test, as described in https://blog.stuffedcow.net/2014/01/x86-memory-disambiguation/
// uses 4B loads and 8B stores to see when/if store forwarding can succeed when sizes are not matched
// pageEnd = push test to the end of (pageEnd) sized page. 0 = just test cacheline
// loadDistance = how far ahead to push the load (for testing aliasing)
// cannot set both pageEnd and loadDistance
void StlfTestMain(uint32_t iterations, int mode, int pageEnd, int loadDistance) {
    TimerStructure timer;
    TimerResult timer_result;
    uint64_t time_diff_ms;
    TimerResult latency;
    TimerResult stlfResults[64][64];
    char *arr; 
    char *allocArr;

    // defaults: grab a couple of cachelines
    int testAlignment = 64, testAllocSize = 128, testOffset = 0;

    if (pageEnd != 0) {
        testAlignment = pageEnd;
        testAllocSize = pageEnd * 2;
        testOffset = pageEnd - 64;
    } else if (loadDistance != 0) {
        testAlignment = 4096;
        testAllocSize = loadDistance + 128; // enough if I ever go to avx-512 loads
    }

    // obtain a couple of cachelines, assuming 64B cacheline size
    allocArr = (char *)common_mem_malloc_aligned(testAllocSize, testAlignment);
    if (allocArr == NULL) {
        fprintf(stderr, "Could not obtain aligned memory\n");
        //timer_result.per_iter_ns = -1;
        return; // timer_result;
    }
    arr = allocArr + testOffset;

    for (int storeOffset = 0; storeOffset < 64; storeOffset++)
        for (int loadOffset = 0; loadOffset < 64; loadOffset++) {
            ((uint32_t *)(arr))[0] = storeOffset;
            ((uint32_t *)(arr))[1] = loadOffset + loadDistance;
            common_timer_start(&timer);
            stlfFunc(iterations, arr);
            common_timer_end(&timer, &(stlfResults[storeOffset][loadOffset]), iterations);
            fprintf(stderr, "Store offset %d, load offset %d: %f ns\n", storeOffset, loadOffset, latency);
        }

    // output as CSV
    for (int loadOffset = 0; loadOffset < 64; loadOffset++) printf(",%d", loadOffset);
    printf("\n");
    for (int storeOffset = 0; storeOffset < 64; storeOffset++) {
        printf("%d", storeOffset);
        for (int loadOffset = 0; loadOffset < 64; loadOffset++) {
            printf(",%f", stlfResults[storeOffset][loadOffset].per_iter_ns);
        }
        printf("\n");
    }
    common_mem_aligned_free( (Ptr64b*) allocArr);
    return;
}
