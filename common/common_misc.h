#ifndef COMMON_MISC_H
#define COMMON_MISC_H
//<includes>
#include "common_common.h"
#if (IS_GCC(ENVTYPE) && IS_ISA_X86(ENVTYPE))
#ifndef WEIRDBUG_CPUID_H_DEFINED
#include <cpuid.h>
#define WEIRDBUG_CPUID_H_DEFINED
#endif
#elif IS_MSVC(ENVTYPE)
#include <intrin.h>
#include <immintrin.h>
#include <windows.h>
#endif

#if IS_GCC(ENVTYPE)
#include <unistd.h>
#elif IS_MSVC(ENVTYPE)
#include <synchapi.h>
#endif



enum COMMON_MISC_CPU_FEATURE_BITFIELD_BITNUMS {
    _COMMON_MISC_CPU_FEATURE_DUMMY_FOR_BIT_0,
    COMMON_MISC_CPU_FEATURE_X86_32,
    COMMON_MISC_CPU_FEATURE_X86_64,
    COMMON_MISC_CPU_FEATURE_MMX,
    COMMON_MISC_CPU_FEATURE_SSE,
    COMMON_MISC_CPU_FEATURE_AVX1,
    COMMON_MISC_CPU_FEATURE_AVX2,
    COMMON_MISC_CPU_FEATURE_AVX512F, // Not sure what this covers
    COMMON_MISC_CPU_FEATURE_AVX512_legacytest, // Old Version


    COMMON_MISC_CPU_FEATURE_ARM_AARCH64,
    COMMON_MISC_CPU_FEATURE_ARM_NEON,
    //COMMON_MISC_CPU_FEATURE_ARM_SVE,
    //...
    //
    _COMMON_MISC_CPU_FEATURE_DUMMY_END
};
//static_assert(_COMMON_MISC_CPU_FEATURE_DUMMY_END <= 32, "Too many feature flags to fit in UINT32_T");
typedef uint32_t CMISC_CPU_FEATURES;

CMISC_CPU_FEATURES common_misc_cpu_features_get(){
    CMISC_CPU_FEATURES features = 0;
#if IS_ISA_X86_64(ENVTYPE) 
    features |= (1 << COMMON_MISC_CPU_FEATURE_X86_32); // All 64 bit x86 CPUs support 32 bit
    features |= (1 << COMMON_MISC_CPU_FEATURE_X86_64);
    //features |= (1 << COMMON_MISC_CPU_FEATURE_MMX); // All 64 bit x86 CPUs support MMX.... right?
    #if IS_GCC(ENVTYPE) 
    //There are a bunch of different options we currently don't seem to need to split by:
    //  "sse2", "sse3", "ssse3", "sse4.1", "sse4.2", "sse4a" and a ton of different 
    // AVX512 versions: "avx512pf", "avx512bw", ...

    if (__builtin_cpu_supports("mmx")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_MMX); 
    if (__builtin_cpu_supports("sse")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_SSE); 
    if (__builtin_cpu_supports("avx")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_AVX1); 
    if (__builtin_cpu_supports("avx2")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_AVX2); 
    if (__builtin_cpu_supports("avx512f")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_AVX512F); 
    // Legacy AVX512F check (incl legacy comments)
    // gcc has no __builtin_cpu_supports for avx512, so check by hand. // This seems no longer true
    // eax = 7 -> extended features, bit 16 of ebx = avx512f
    uint32_t cpuidEax, cpuidEbx, cpuidEcx, cpuidEdx;
    __cpuid_count(7, 0, cpuidEax, cpuidEbx, cpuidEcx, cpuidEdx);
    if (cpuidEbx & (1UL << 16)) {
        features |= (1 << COMMON_MISC_CPU_FEATURE_AVX512_legacytest); 
    }
    #elif IS_MSVC(ENVTYPE)
    //MSVC doesn't seem to have any abstraction interface like gcc's
    //I'm implicitly assuming MMX support for x86-64:
    features |= (1 << COMMON_MISC_CPU_FEATURE_MMX); 
    int cpuid_data[4];

    // cpuid_data[0] = eax, [1] = ebx, [2] = ecx, [3] = edx
    __cpuidex(cpuid_data, 1, 0);
    
    // EDX bit 25 = SSE
    if (cpuid_data[3] & (1UL << 25)) { // Testing for SSE
        features |= (1 << COMMON_MISC_CPU_FEATURE_SSE); 
    }
    if (cpuid_data[2] & (1UL << 28)) { // Testing for AVX
        features |= (1 << COMMON_MISC_CPU_FEATURE_AVX1); 
    }

    __cpuidex(cpuid_data, 7, 0);
    if (cpuid_data[1] & (1UL << 16)) {// Testing for AVX512
        features |= (1 << COMMON_MISC_CPU_FEATURE_AVX512_legacytest); 
    }
    #endif /*compiler type*/
#elif IS_ISA_X86_i686(ENVTYPE) 
    features |= (1 << COMMON_MISC_CPU_FEATURE_X86_32);
    #if IS_GCC(ENVTYPE) 
    //There are a bunch of different options we currently don't seem to need to split by.

    if (__builtin_cpu_supports("mmx")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_MMX); 
    if (__builtin_cpu_supports("sse")>0)
        features |= (1 << COMMON_MISC_CPU_FEATURE_SSE); 
    #elif IS_MSVC(ENVTYPE)
    int cpuid_data[4];
    //MSVC doesn't seem to have any abstraction interface like gcc's
    // cpuid_data[0] = eax, [1] = ebx, [2] = ecx, [3] = edx
    __cpuidex(cpuid_data, 1, 0);
    
    // EDX bit 25 = SSE
    if (cpuid_data[3] & (1UL << 23)) { // Testing for SSE
        features |= (1 << COMMON_MISC_CPU_FEATURE_MMX); 
    }
    
    if (cpuid_data[3] & (1UL << 25)) { // Testing for SSE
        features |= (1 << COMMON_MISC_CPU_FEATURE_SSE); 
    }
    #endif /*compiler type*/
#elif IS_ISA_AARCH64(ENVTYPE)
// Honestly, I can't find a way to get the features supported so I'm just gonna
// assume NEON is and YOLO for now
    features |= (1 << COMMON_MISC_CPU_FEATURE_ARM_AARCH64); 
    features |= (1 << COMMON_MISC_CPU_FEATURE_ARM_NEON); 
    
#endif /*IS_ISA_******(ENVTYPE)*/
    
    return features;
}

#define COMMON_MISC_CHECK_CPU_FEATURE(features, wanted_feature) ((features & wanted_feature) != 0)


inline void common_misc_sleep_sec(unsigned int seconds){
#if IS_GCC(ENVTYPE)
    sleep(seconds);
#elif IS_MSVC(ENVTYPE)
    Sleep(seconds*1000);
#endif
}



#endif /*COMMON_MISC_H*/ 