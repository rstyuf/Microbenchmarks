
#ifndef COMMON_MSR_SET_INTEL_ARCH_H
#define COMMON_MSR_SET_INTEL_ARCH_H
#include "../common_msr.h"

#define MSR_UNC_PERF_GLOBAL_CTRL 0xe01
#define MSR_UNC_PERF_FIXED_CTRL 0x394
#define MSR_UNC_PERF_FIXED_CTR 0x395
#define MSR_IA32_FIXED_CTR1 0x30A
#define IA32_PERF_GLOBAL_CTRL  0x38F 
#define IA32_FIXED_CTR_CTRL  0x38D


void intel_arch_msrs_enable_uncore_global_fixed_cntrs(MsrDescriptor msrd){
 // Enabling global uncore counter: Needs to be enabled to be able to turn on any uncore counter
    uint64_t current = common_msr_read(msrd, MSR_UNC_PERF_GLOBAL_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_GLOBAL_CTRL, ((1UL << 29) | current));
    
    // Enabling local uncore counters: Enables the uncore clock counter ("FIXED_CTR")
    current = common_msr_read(msrd, MSR_UNC_PERF_FIXED_CTRL);
    common_msr_write(msrd, MSR_UNC_PERF_FIXED_CTRL, ((1UL << 22) | current));
}

void intel_arch_msrs_enable_core_global_fixed_cntrs(MsrDescriptor msrd){
    //core: Needs to be enabled on each core which is relevant. 
    uint64_t current = common_msr_read(msrd, IA32_FIXED_CTR_CTRL); //Enables all the "Fixed" core counters to start counting
    common_msr_write(msrd, IA32_FIXED_CTR_CTRL, (0x0000000000000333 | current));
    current = common_msr_read(msrd, IA32_PERF_GLOBAL_CTRL); // Enables in general all the counters in the core
    common_msr_write(msrd, IA32_PERF_GLOBAL_CTRL, (0x000000070000000F | current));
}

#endif /* COMMON_MSR_SET_INTEL_ARCH_H */