#define ALTERNATE_DATALOGGER 1
#define QUIET_OUTPUT true
//#define OUTPRINTFD stdout
#define ALTERNATE_TIMER_MSRS 3
#define COMPOUND_TEST true
#include "../common/common_common.h"
#include "../common/common_msr.h"
#include "../common/msr_sets/intel_arch_msrs.h"
//#include "../common/common_timer.h"
#include "../common/alternative_implementations/commonalt_timer_withmsrs.h"
#include "../MemoryLatency/MemoryLatency.c"
#include "../CoherencyLatency/CoherencyLatency.cpp"
#include "../MemoryBandwidth/MemoryBandwidth.c"

#define MAX_CORES (64)
#define MAX_TEST_NAME_LEN (32)

//#define COMP
//#include "Windows.h"
#include "stdlib.h"
int my_default_test_sizes[] = { 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 600, 768, 1024, 1536, 2048,
                               3072, 4096, 5120, 6144, 8192, 10240, 12288, 16384};//, 24567, 32768, 65536, 98304,
//                               131072, 262144, 393216, 524288, 1048576 }; //2097152 };
//*
struct core_run_conf_t{
    int core_id; //-1 is end?
    uint64_t testsuite; // bitfield of tests
};

struct args_to_single_thread_test_t{
    TimerStructure *timer;
    DataLog *dlog;
    uint32_t * dataArr;
    int* test_sizes;
    int testSizeCount;
};
enum TESTSUITE_BITS_e {
    TESTSUITE_BITS_MEMLAT_C_HUGEPG,
    TESTSUITE_BITS_MEMLAT_C_REGPG,
    TESTSUITE_BITS_MEMLAT_ASM_HUGEPG,
    TESTSUITE_BITS_MEMLAT_ASM_REGPG,
    TESTSUITE_BITS_MEMLAT_TLB_REGPG,
    TESTSUITE_BITS_MEMLAT_TLBRAW_REGPG,
    TESTSUITE_BITS_MEMLAT_MLPTEST_REGPG,
    TESTSUITE_BITS_MEMLAT_SLFT_REGPG,
    TESTSUITE_BITS_C2C_BOUNCE,
    TESTSUITE_BITS_C2C_OWNED,
    TESTSUITE_BITS_MEMBW_AUTOTHREADS_SHARED,
    TESTSUITE_BITS_MEMBW_AUTOTHREADS_PRIVATE,
};


int SingleThreadedTest(TimerStructure* timer, DataLog* dlog, int* test_sizes, int testSizeCount, int max_test_size_mb, uint32_t *dataArr, uint64_t core_testsuite){
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_C_HUGEPG)) != 0){
        printf("Running MLat C_HGpg Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestC_HugePg");
        #endif
        RunAllLatencyTestSizes(test_sizes, testSizeCount, dataArr, 0, 0, RunLatencyTest, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_C_REGPG)) != 0){
        printf("Running MLat C_Rpg Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestC");
        #endif
        RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunLatencyTest, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_ASM_HUGEPG)) != 0){
        printf("Running MLat ASM_HGpg Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestAsm_HugePg");    
        #endif
        RunAllLatencyTestSizes(test_sizes, testSizeCount, dataArr, 0, 0, RunAsmLatencyTest, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_ASM_REGPG)) != 0){
        printf("Running MLat ASM_Rpg Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestAsm");    
        #endif
        RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunAsmLatencyTest, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_TLB_REGPG)) != 0){
            printf("Running MLat TLB-Rpg Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestTLB");    
        #endif
        RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunTlbLatencyTest, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_TLBRAW_REGPG)) != 0){
        printf("Running MLat TLBRAW-Rpg Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestRawTLB");    
        #endif
        RunAllLatencyTestSizes(test_sizes, testSizeCount, NULL, 0, 0, RunTlbRawLatencyTest, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_SLFT_REGPG)) != 0){
        printf("Running MLat STLF Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_STLF, "MemLat_STLF");
            common_datalogger_swap_outfd(dlog, "outfd_MemLat_STLF.csv", true);
        #endif
        StlfTestMain(ITERATIONS, 1, 0, 0, timer, dlog);
    }
    if ((core_testsuite & (1 << TESTSUITE_BITS_MEMLAT_MLPTEST_REGPG)) != 0){
        printf("Running MLat MLP Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(dlog, DATA_LOGGER_LOG_TYPES_MLP, "MemLat_MLP");
            common_datalogger_swap_outfd(dlog, "outfd_MemLat_MLP.csv", true);
        #endif
        MlpTestMain(32, test_sizes, testSizeCount, timer, dlog);
    }
}



int test_runner_main(bool msr_perf, int large_pg, struct core_run_conf_t* core_profiles, uint64_t testsuite, int max_run_sizes_mb, char* testname){
    MsrDescriptor msrd = common_msr_open();
    intel_arch_msrs_enable_uncore_global_fixed_cntrs(msrd);

    TimerStructure timer;
    DataLog dlog = {0};
    /// 
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_all(&dlog, testname);
        common_datalogger_swap_outfd(&dlog, "outfd_start", true);
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
    if (max_run_sizes_mb != 0 && max_test_size > (max_run_sizes_mb*1024)){
        max_test_size = max_run_sizes_mb*1024;
    }
    //printf("#### %d :: %d\n", testSizeCount, max_test_size);

    uint32_t * dataArr = NULL;
    if (large_pg > 0) {
        dataArr = (uint32_t* ) common_mem_malloc_special(max_test_size*1024, 0, true, NULL);
        if (dataArr == NULL && large_pg == 1) {
            printf("Large Page allocation failed and is configured to be mandatory. Make sure that we are running as Administrator and Large Pages are enabled. If this persists, try a reboot.\n");
            return -1;
        } else if (dataArr == NULL && large_pg == 2) {
            printf("Large Page allocation failed. Make sure that we are running as Administrator and Large Pages are enabled. If this persists, try a reboot. Continuing without Large Page tests.\n");
            if ((testsuite & (1 << TESTSUITE_BITS_MEMLAT_C_HUGEPG)) != 0){
                testsuite = testsuite | (1 << TESTSUITE_BITS_MEMLAT_C_REGPG); // Enabling regular test instead
            }
            if ((testsuite & (1 << TESTSUITE_BITS_MEMLAT_ASM_HUGEPG)) != 0){
                testsuite = testsuite | (1 << TESTSUITE_BITS_MEMLAT_ASM_REGPG); // Enabling regular test instead
            }
            testsuite = testsuite & (~(1 << TESTSUITE_BITS_MEMLAT_C_HUGEPG));
            testsuite = testsuite & (~(1 << TESTSUITE_BITS_MEMLAT_ASM_HUGEPG));
        }
    }
    for (int cpid = 0; core_profiles[cpid].core_id >= 0 && cpid < MAX_CORES; cpid++){
        common_threading_set_affinity(core_profiles[cpid].core_id);
        common_datalogger_set_predef(&dlog, core_profiles[cpid].core_id, 1);
        printf("\nRunning core %d tests:\n", core_profiles[cpid].core_id);
        intel_arch_msrs_enable_core_global_fixed_cntrs(msrd); // TODO: How to handle migration?
        SingleThreadedTest(&timer, &dlog, test_sizes, testSizeCount, max_run_sizes_mb, dataArr, (testsuite & core_profiles[cpid].testsuite));
    }
    common_threading_set_affinity(core_profiles[0].core_id);
    common_datalogger_set_predef(&dlog, core_profiles[0].core_id, 1);
    intel_arch_msrs_enable_core_global_fixed_cntrs(msrd); // TODO: How to handle migration?
    if ((testsuite & (1 << TESTSUITE_BITS_C2C_BOUNCE)) != 0){
        printf("Running C2C Bounce Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_C2C, "C2C-Bounce");    
            common_datalogger_swap_outfd(&dlog, "outfd_C2C_Bounce.csv", true);
        #endif
        CoherencyTestMain(1, COHERENCYLAT_DEFAULT_ITERATIONS, RunCoherencyBounceTest, &timer, &dlog);
    }
    if ((testsuite & (1 << TESTSUITE_BITS_C2C_OWNED)) != 0){
        printf("Running C2C Owned Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_C2C, "C2C-Owned");    
            common_datalogger_swap_outfd(&dlog, "outfd_C2C_Owned.csv", true);
        #endif
        CoherencyTestMain(1, COHERENCYLAT_DEFAULT_ITERATIONS, RunCoherencyOwnedTest, &timer, &dlog);
    }
    int cpuCount = common_threading_get_num_cpu_cores();
    MemBWFunc bw_func = asm_read;
    _membw_get_default_runfunc(&bw_func);
    if ((testsuite & (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_SHARED)) != 0){
        printf("Running MemoryBW Autothreads Shared Array Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMBW, "MBW-Autothreads-shared");    
            common_datalogger_swap_outfd(&dlog, "outfd_MBW-Autothreads-shared.csv", true);
        #endif
        RunMemoryBandwidthAutoThreadsTest(test_sizes, testSizeCount, cpuCount, default_gbToTransfer, /*sleepTime*/ 1,  /*shared*/ 1, /*nopBytes*/ 0, /*branchInterval*/0, 0, 0, NULL,  bw_func, NULL, &timer, &dlog);
    }
     if ((testsuite & (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_PRIVATE)) != 0){
        printf("Running MemoryBW Autothreads Private Array Test\n");
        #if ALTERNATE_DATALOGGER == 1
            common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMBW, "MBW-Autothreads-private");    
            common_datalogger_swap_outfd(&dlog, "outfd_MBW-Autothreads-private.csv", true);
        #endif
        RunMemoryBandwidthAutoThreadsTest(test_sizes, testSizeCount, cpuCount, default_gbToTransfer, /*sleepTime*/ 1,  /*shared*/ 0, /*nopBytes*/ 0, /*branchInterval*/0, 0, 0, NULL,  bw_func, NULL, &timer, &dlog);
    }

    printf("\nDone, Cleaning up\n");
    common_datalogger_close_all(&dlog);
    char* datalog_folder_path = common_datalogger_get_testset_folder_name(&dlog);
    common_msr_close(msrd);
    common_mem_special_free((Ptr64b*)dataArr);
    char compress_cmd_string[256] = {0};
    snprintf(compress_cmd_string, 255, "tar -cf  %s.zip %s", datalog_folder_path, datalog_folder_path);
    system(compress_cmd_string);
}

int main(int argc, char *argv[]) {
    bool msr_perf = true;
    int large_pg = 2; // 1 = yes, 2 = besteffort, 0 = no  
    struct core_run_conf_t core_profiles[MAX_CORES] = {0};
    uint64_t testsuite = 0; 
    int max_run_sizes = 0;
    char testname[MAX_TEST_NAME_LEN] = {0};
    // Set defaults ***
    // Get user configuration ***
    //
    printf("Make sure to Run as Administrator\n");
    //Enabling tests as default:
    int testsuite_full = 0, testsuite_mid = 0, testsuite_min = 0;
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_C_HUGEPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_C_REGPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_ASM_HUGEPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_ASM_REGPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_TLB_REGPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_TLBRAW_REGPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_MLPTEST_REGPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMLAT_SLFT_REGPG); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_C2C_BOUNCE); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_C2C_OWNED); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_SHARED); 
    testsuite_full = testsuite_full | (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_PRIVATE); 

    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_C_HUGEPG); 
    //testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_C_REGPG); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_ASM_HUGEPG); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_ASM_REGPG); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_TLB_REGPG); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_TLBRAW_REGPG); 
    //testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_MLPTEST_REGPG); 
    //testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMLAT_SLFT_REGPG); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_C2C_BOUNCE); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_C2C_OWNED); 
    testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_SHARED); 
    //testsuite_mid = testsuite_mid | (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_PRIVATE);

    testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_C_HUGEPG); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_C_REGPG); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_ASM_HUGEPG); 
    testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_ASM_REGPG); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_TLB_REGPG); 
    testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_TLBRAW_REGPG); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_MLPTEST_REGPG); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMLAT_SLFT_REGPG); 
    testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_C2C_BOUNCE); 
    testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_C2C_OWNED); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_SHARED); 
    //testsuite_min = testsuite_min | (1 << TESTSUITE_BITS_MEMBW_AUTOTHREADS_PRIVATE);

    testsuite = testsuite_full;

    strncpy(testname, "TestSetA", MAX_TEST_NAME_LEN-1);
    core_profiles[0].core_id = 0;
    core_profiles[0].testsuite = -1;
    core_profiles[1].core_id = common_threading_get_num_cpu_cores()-1; //TMP
    core_profiles[1].testsuite = -1;
    core_profiles[2].core_id = -1;
    int last_defined_core_profile = -1;
    /////////////////
    for (int argIdx = 1; argIdx < argc; argIdx++) {
        if (*(argv[argIdx]) == '-') {
            char *arg = argv[argIdx] + 1;

            if (strncmp(arg, "name", 4) == 0) {
                argIdx++;
                char *new_testname = argv[argIdx];
                strncpy(testname, new_testname, MAX_TEST_NAME_LEN-1);
            } else if (strncmp(arg, "maxsizemb", 9) == 0) {
                argIdx++;
                max_run_sizes = atoi(argv[argIdx]);
                fprintf(stderr, "Will not exceed %u MB\n", max_run_sizes);
            } else if (strncmp(arg, "test", 4) == 0) {
                argIdx++;
                char *testArg = argv[argIdx];
		        if (strncmp(testArg, "full", 4) == 0) {
                    testsuite = testsuite_full;
                } else if (strncmp(testArg, "mid", 3) == 0) {
                    testsuite = testsuite_mid;
                } else if (strncmp(testArg, "min", 3) == 0) {
                    testsuite = testsuite_min;
                } else{
                    testsuite = atoi(testArg);
                }
            } else if (strncmp(arg, "flargepg", 8) == 0) {
                large_pg = 1;
            } else if (strncmp(arg, "nolargepg", 9) == 0) {
                large_pg = 0;
            } else if (strncmp(arg, "wlargepg", 8) == 0) {
                large_pg = 2;
            } else if (strncmp(arg, "nomsr", 5) == 0) {
                msr_perf = false;
            } else if (strncmp(arg, "msr", 3) == 0) {
                msr_perf = true;
            } else if (strncmp(arg, "c", 1) == 0 || strncmp(arg, "core", 4) == 0) {
                argIdx++;
                char *testArg1 = argv[argIdx];
                last_defined_core_profile ++;
                core_profiles[last_defined_core_profile].core_id = atoi(testArg1);
                core_profiles[last_defined_core_profile + 1].core_id = -1;
                if (argIdx +1 < argc){
                    char *testArg2 = argv[argIdx+1];
                    if (*(testArg2) == '-'){
                        core_profiles[last_defined_core_profile].testsuite = -1;
                    } else if (strncmp(testArg2, "full", 4) == 0) {
                        core_profiles[last_defined_core_profile].testsuite = testsuite_full;
                    } else if (strncmp(testArg2, "mid", 3) == 0) {
                        core_profiles[last_defined_core_profile].testsuite = testsuite_mid;
                    } else if (strncmp(testArg2, "min", 3) == 0) {
                        core_profiles[last_defined_core_profile].testsuite = testsuite_min;
                    } else{
                        core_profiles[last_defined_core_profile].testsuite = atoi(testArg2);
                    }
                } else {
                        core_profiles[last_defined_core_profile].testsuite = -1;
                }
            } else {
                fprintf(stderr, "Unrecognized option: %s\n", arg);
            }
        }
    }

    test_runner_main(msr_perf, large_pg, core_profiles, testsuite, max_run_sizes, testname);
}

/*
int test_runner_main(bool msr_perf, bool large_pg, struct core_run_conf_t* core_profiles, uint64_t testsuite, int max_run_sizes){
    MsrDescriptor msrd = common_msr_open();
    //printf("Which core should the test be run from? Core # as seen by OS:");
    //int coreid;
    //scanf("%d", &coreid);
    common_threading_set_affinity(coreid);
    intel_arch_msrs_enable_uncore_global_fixed_cntrs(msrd);
    intel_arch_msrs_enable_core_global_fixed_cntrs(msrd); // TODO: How to handle migration?
    
    //printf("Starting...\n");

    //int test_sizes[6] = {4, 32, 40, 48, 56, x};
    TimerStructure timer;
    DataLog dlog = {0};
    /// 
    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_all(&dlog, "testingsetA");
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MEMLAT, "MemLat_LatTestC_HugePg");
        common_datalogger_swap_outfd(&dlog, "outfd_start", true);
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
        common_datalogger_swap_outfd(&dlog, "outfd_MemLat_STLF.csv", true);
    #endif
    StlfTestMain(ITERATIONS, 1, 0, 0, &timer, &dlog);

    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_MLP, "MemLat_MLP");
        common_datalogger_swap_outfd(&dlog, "outfd_MemLat_MLP.csv", true);
    #endif
    MlpTestMain(32, test_sizes, testSizeCount, &timer, &dlog);
* /

    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_C2C, "C2C-Bounce");    
        common_datalogger_swap_outfd(&dlog, "outfd_C2C_Bounce.csv", true);
    #endif
    CoherencyTestMain(1, COHERENCYLAT_DEFAULT_ITERATIONS, RunCoherencyBounceTest, &timer, &dlog);

    #if ALTERNATE_DATALOGGER == 1
        common_datalogger_init_subtest(&dlog, DATA_LOGGER_LOG_TYPES_C2C, "C2C-Owned");    
        common_datalogger_swap_outfd(&dlog, "outfd_C2C_Owned.csv", true);
    #endif
    CoherencyTestMain(1, COHERENCYLAT_DEFAULT_ITERATIONS, RunCoherencyOwnedTest, &timer, &dlog);


    printf("\ncleaning up\n");
    common_datalogger_close_all(&dlog);
    common_msr_close(msrd);
    common_mem_special_free((Ptr64b*)dataArr);
}
*/
