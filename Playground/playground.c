#define ALTERNATE_DATALOGGER 1
#define ALTERNATE_TIMER_MSRS 3
#define COMPOUND_TEST true
#include "../common/common_common.h"
#include "../common/common_msr.h"
//#include "../common/common_timer.h"
#include "../common/alternative_implementations/commonalt_timer_withmsrs.h"
#include "../MemoryLatency/MemoryLatency.c"
//#define COMP
//#include "Windows.h"
#include "stdlib.h"

#define MSR_UNC_PERF_GLOBAL_CTRL 0xe01
#define MSR_UNC_PERF_FIXED_CTRL 0x394
#define MSR_UNC_PERF_FIXED_CTR 0x395
#define MSR_IA32_FIXED_CTR1 0x30A
#define IA32_PERF_GLOBAL_CTRL  0x38F 
#define IA32_FIXED_CTR_CTRL  0x38D

//*
int main(int argc, char *argv[]) {
    MsrDescriptor msrd = common_msr_open();
    int x;
    scanf("%d", &x);
    // Enabling global counter : Needs to be enabled to be able to turn on any uncore counter
    uint64_t current = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    uint64_t current2;// = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_GLOBAL_CTRL, ((1UL << 29) | current));
    
    // Enabling local counters
    //uncore : Enables the uncore clock counter ("FIXED_CTR")
    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_FIXED_CTRL, ((1UL << 22) | current));
    //core: Needs to be enabled on each core which is relevant. 
    current = common_msr_read(msrd, IA32_FIXED_CTR_CTRL); //Enables all the "Fixed" core counters to start counting
    common_msr_write(msrd, IA32_FIXED_CTR_CTRL, (0x0000000000000333 | current));
    current = common_msr_read(msrd, IA32_PERF_GLOBAL_CTRL); // Enables in general all the counters in the core
    common_msr_write(msrd, IA32_PERF_GLOBAL_CTRL, (0x000000070000000F | current));

    printf("Starting...\n");

    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTR);
    current2 = common_msr_read(msrd, MSR_IA32_FIXED_CTR1);

    Sleep(1000);
    uint64_t second = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTR);
    uint64_t second2 = common_msr_read(msrd, MSR_IA32_FIXED_CTR1);
    printf("Results were %lu and %lu meaning diff of %lu\n", current, second, second-current);
    printf("Results were 0x%lx and 0x%lx meaning diff of 0x%lx\n", current, second, second-current);
    
    printf("IAA Results were %lu and %lu meaning diff of %lu\n", current2, second2, second2-current2);
    printf("IA Results were 0x%lx and 0x%lx meaning diff of 0x%lx\n", current2, second2, second2-current2);

    int test_sizes[6] = {4, 32, 40, 48, 56, x};
    TimerStructure timer;
    DataLog dlog = {0};
    /// 
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_all(&dlog, "testingsetA");
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "myTestLat");

    #endif
    ///
    #if ALTERNATE_TIMER_MSRS > 2
        timer.msrd = msrd;
        timer.msr_tracker[0].usage_type = 1;
        timer.msr_tracker[0].value_fieldsize_bits = 44;
        timer.msr_tracker[0].specific_core = 0-1;
        timer.msr_tracker[0].addr = MSR_UNC_PERF_FIXED_CTR;
        timer.msr_tracker[1].usage_type = 2;
        timer.msr_tracker[1].value_fieldsize_bits = 44;
        timer.msr_tracker[1].specific_core = 0-1;
        timer.msr_tracker[1].addr = MSR_IA32_FIXED_CTR1;

    #endif
    uint32_t * dataArr = (uint32_t* ) common_mem_malloc_special(56*1024, 0, false, COMMON_MEM_ALLOC_NUMA_DISABLED);
    if (dataArr == NULL) return -1;

    RunAllLatencyTestSizes(test_sizes, 6, dataArr, 0, 0, RunLatencyTest, &timer, &dlog);
    
    #ifdef COMMON_TIMER_MSR_H
    TimerResult tres;//= {0};
    printf("Starting round 2...\n");
    common_timer_start(&timer);
    Sleep(1000);
    common_timer_end(&timer, &tres);
    common_timer_result_process_iterations(&tres, 1);
    common_timer_result_fprint(&tres, stdout);
    
    #endif
    printf("\ncleaning up\n");
    common_datalogger_close_all(&dlog);
}
/*
int main(int argc, char *argv[]) {
    MsrDescriptor msrd = common_msr_open();
    // Enabling global counter
    int x;
    scanf("%d", &x);

    uint64_t current = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    uint64_t current2;// = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_GLOBAL_CTRL, ((1UL << 29) | current));
    
    // Enabling local counters
    //uncore
    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_FIXED_CTRL, ((1UL << 22) | current));
    //core:
    current = common_msr_read(msrd, IA32_FIXED_CTR_CTRL);
    common_msr_write(msrd, IA32_FIXED_CTR_CTRL, (0x0000000000000333 | current));
    current = common_msr_read(msrd, IA32_PERF_GLOBAL_CTRL);
    common_msr_write(msrd, IA32_PERF_GLOBAL_CTRL, (0x000000070000000F | current));
    /*
    printf("Starting...\n");

    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTR);
    current2 = common_msr_read(msrd, MSR_IA32_FIXED_CTR1);

    Sleep(1000);
    uint64_t second = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTR);
    uint64_t second2 = common_msr_read(msrd, MSR_IA32_FIXED_CTR1);
    printf("Results were %lu and %u meaning diff of %lu\n", current, second, second-current);
    printf("Results were 0x%lx and 0x%x meaning diff of 0x%lx\n", current, second, second-current);
    
    printf("IA Results were %lu and %u meaning diff of %lu\n", current2, second2, second2-current2);
    printf("IA Results were 0x%lx and 0x%x meaning diff of 0x%lx\n", current2, second2, second2-current2);

    // * /
    int test_sizes[6] = {4, 32, 40, 48, 56, x};
    DataLog dlog = {0};
    TimerStructure timer;

    /// 
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_all(&dlog, "testingsetA");
        snprintf(dlog.active_subtest_name, STRUCT_STRING_MAX, "Testing");
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "myTestLat");

    #endif
    #if ALTERNATE_TIMER_MSRS > 2
        timer.msrd = msrd;
        timer.msr_tracker[0].usage_type = 1;
        timer.msr_tracker[0].value_fieldsize_bits = 44;
        timer.msr_tracker[0].specific_core = 0-1;
        timer.msr_tracker[0].addr = MSR_UNC_PERF_FIXED_CTR;
        timer.msr_tracker[1].usage_type = 2;
        timer.msr_tracker[1].value_fieldsize_bits = 44;
        timer.msr_tracker[1].specific_core = 0-1;
        timer.msr_tracker[1].addr = MSR_IA32_FIXED_CTR1;

    #endif
    ///
    uint32_t * dataArr = (uint32_t* ) common_mem_malloc_special(56*1024, 0, false, COMMON_MEM_ALLOC_NUMA_DISABLED);
    if (dataArr == NULL) return -1;

    RunAllLatencyTestSizes(test_sizes, 6, dataArr, 0, 0, RunLatencyTest, &timer, &dlog);

    #ifdef COMMON_TIMER_MSR_H
   /* TimerResult tres;//= {0};
    printf("Starting round 2...\n");
    common_timer_start(&timer);
    Sleep(1000);
    common_timer_end(&timer, &tres);
    common_timer_result_process_iterations(&tres, 1);
    common_timer_result_fprint(&tres, stdout);
    * /
   #endif
    common_datalogger_close_all(&dlog);
}*/



/*
int main(int argc, char *argv[]) {
    MsrDescriptor msrd = common_msr_open();
    // Enabling global counter
    uint64_t current = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    uint64_t current2;// = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_GLOBAL_CTRL, ((1UL << 29) | current));
    
    // Enabling local counter
    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_FIXED_CTRL, ((1UL << 22) | current));
    printf("Starting...\n");

    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTR);
    current2 = common_msr_read(msrd, MSR_IA32_FIXED_CTR1);

    Sleep(1000);
    uint64_t second = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTR);
    uint64_t second2 = common_msr_read(msrd, MSR_IA32_FIXED_CTR1);
    printf("Results were %u and %u meaning diff of %u\n", current, second, second-current);
    printf("Results were 0x%x and 0x%x meaning diff of 0x%x\n", current, second, second-current);
    
    printf("IAA Results were %u and %u meaning diff of %u\n", current2, second2, second2-current2);
    printf("IA Results were 0x%x and 0x%x meaning diff of 0x%x\n", current2, second2, second2-current2);

    int test_sizes[5] = {4, 32, 40, 48, 56};
    DataLog dlog = {0};
    /// 
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_all(&dlog, "testingsetA");
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "myTestLat");

    #endif
    ///
    TimerStructure timer;
    uint32_t * dataArr = (uint32_t* ) common_mem_malloc_special(56*1024, 0, false, COMMON_MEM_ALLOC_NUMA_DISABLED);
    if (dataArr == NULL) return -1;

    RunAllLatencyTestSizes(test_sizes, 5, dataArr, 0, 0, RunLatencyTest, &timer, &dlog);
}
*/