#define ALTERNATE_DATALOGGER 1
#define ALTERNATE_TIMER_MSRS 3
#define COMPOUND_TEST true
#include "../common/common_common.h"
#include "../common/common_msr.h"
#include "../common/msr_sets/intel_arch_msrs.h"
//#include "../common/common_timer.h"
#include "../common/alternative_implementations/commonalt_timer_withmsrs.h"
#include "../MemoryLatency/MemoryLatency.c"
#include "../CoherencyLatency/CoherencyLatency.cpp"
//#include "../MemoryBandwidth/MemoryBandwidth.cpp"

//#define COMP
//#include "Windows.h"
#include "stdlib.h"
int my_default_test_sizes[] = { 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 600, 768, 1024, 1536, 2048,
                               3072, 4096, 5120, 6144, 8192, 10240, 12288, 16384};//, 24567, 32768, 65536, 98304,
//                               131072, 262144, 393216, 524288, 1048576 }; //2097152 };
//*
struct args_to_single_thread_test_t{
    TimerStructure *timer;
    DataLog *dlog;
    uint32_t * dataArr;
    int* test_sizes;
    int testSizeCount;
};
int SingleThreadedTest(){
    
}

int main(int argc, char *argv[]) {
    printf("Make sure to Run as Administrator\n");
    MsrDescriptor msrd = common_msr_open();
    printf("Which core should the test be run from? Core # as seen by OS:");
    int coreid;
    scanf("%d", &coreid);
    common_threading_set_affinity(coreid);
    intel_arch_msrs_enable_uncore_global_fixed_cntrs(msrd);
    intel_arch_msrs_enable_core_global_fixed_cntrs(msrd); // TODO: How to handle migration?
    
    printf("Starting...\n");

    //int test_sizes[6] = {4, 32, 40, 48, 56, x};
    TimerStructure timer;
    DataLog dlog = {0};
    /// 
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_all(&dlog, "testingsetA");
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestC_HugePg");
    #endif
    ///
    #if ALTERNATE_TIMER_MSRS > 2
        timer.msrd = msrd;
        timer.msr_tracker[0].usage_type = 1;
        timer.msr_tracker[0].value_fieldsize_bits = 44;
        timer.msr_tracker[0].specific_core = 0-1;
        timer.msr_tracker[0].addr = MSR_UNC_PERF_FIXED_CTR;
        timer.msr_tracker[1].usage_type = 1;
        timer.msr_tracker[1].value_fieldsize_bits = 44;
        timer.msr_tracker[1].specific_core = 0-1;
        timer.msr_tracker[1].addr = MSR_IA32_FIXED_CTR1;

    #endif
    int* test_sizes = my_default_test_sizes;
    uint32_t testSizeCount = sizeof(my_default_test_sizes) / sizeof(int);
    uint64_t max_test_size = test_sizes[testSizeCount-1];
    //printf("#### %d :: %d\n", testSizeCount, max_test_size);

    uint32_t * dataArr = (uint32_t* ) common_mem_malloc_special(max_test_size*1024, 0, true, COMMON_MEM_ALLOC_NUMA_DISABLED);
    if (dataArr == NULL) return -1;

    RunAllLatencyTestSizes(test_sizes, testSizeCount, dataArr, 0, 0, RunLatencyTest, &timer, &dlog);
    
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestAsm_HugePg");    
    #endif
    RunAllLatencyTestSizes(test_sizes, testSizeCount, dataArr, 0, 0, RunAsmLatencyTest, &timer, &dlog);
    
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestAsm");    
    #endif
    RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunAsmLatencyTest, &timer, &dlog);
 /*   
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestTLB");    
    #endif
    RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunTlbLatencyTest, &timer, &dlog);
    
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestRawTLB");    
    #endif
    RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunTlbRawLatencyTest, &timer, &dlog);
    
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_STLF, "MemLat_STLF");    
    #endif
    StlfTestMain(ITERATIONS, 1, 0, 0, &timer, &dlog);

    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MLP, "MemLat_MLP");    
    #endif
    MlpTestMain(32, test_sizes, testSizeCount, &timer, &dlog);

*/
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_C2C, "C2C-Bounce");    
    #endif
    CoherencyTestMain(1, COHERENCYLAT_DEFAULT_ITERATIONS, RunCoherencyBounceTest, &timer, &dlog);

    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_C2C, "C2C-Owned");    
    #endif
    CoherencyTestMain(1, COHERENCYLAT_DEFAULT_ITERATIONS, RunCoherencyOwnedTest, &timer, &dlog);


    printf("\ncleaning up\n");
    common_datalogger_close_all(&dlog);
    common_msr_close(msrd);
    common_mem_special_free((Ptr64b*)dataArr);
}

