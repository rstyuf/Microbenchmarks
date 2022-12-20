#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H


//idea: to be able to hold one variable that functions as a sort of decimal equiv
// of bitfields to hold all env type variations (MSVC/GCC, OS, ISA, calling conventions...)
//For instance: XYZW where:
//               X represents build env {1:MSVC, 2:GCC},
//               Y represents ISA {1:x86-64, 2:x86 (i686), 3: ARM Aarch64, 4: other ARM, 0: Unknown}, 
#define BUILDENV_GROUP_RANGE_SIZE (1000)
#define WINDOWS_MSVC_RANGESTART (BUILDENV_GROUP_RANGE_SIZE)
#define WINDOWS_MSVC (WINDOWS_MSVC_RANGESTART + 1)
#define POSIX_GCC_RANGESTART (WINDOWS_MSVC_RANGESTART + BUILDENV_GROUP_RANGE_SIZE)
#define POSIX_GCC (POSIX_GCC_RANGESTART + 1)

#define ISA_GROUP_RANGE_SIZE (100)
#define ISA_X86_64_VAL   (1)
#define ISA_X86_i686_VAL (2)
#define ISA_AARCH64_VAL  (3)
#define ISA_ARM32_VAL    (4)
#define ISA_UNKNOWN_VAL  (0)


// Ptr64b definition: generic datatype used to point to things
// Note that the type is signed in Windows MSVC and unsigned in GCC
// Maybe worth aligning GCC to signed int (int64_t)? Does it matter?
#ifdef _MSC_VER

    #include <windows.h>
    #ifdef _M_ARM64 // Hopefully the documentation is right and this doesn't include ARM64EC
        #define ISA_VAL (ISA_AARCH64_VAL)    
    #elif _M_ARM    // is this even relevant for windows/MSVC?
        #define ISA_VAL (ISA_ARM32_VAL)    
    #elif _M_X64 
        #define ISA_VAL (ISA_X86_64_VAL)    
    #elif _M_IX86
        #define ISA_VAL (ISA_X86_i686_VAL)   
    #else
        #define ISA_VAL (ISA_UNKNOWN_VAL)    
    #endif
    #define ENVTYPE (WINDOWS_MSVC + (ISA_VAL*ISA_GROUP_RANGE_SIZE))
    typedef LONG64 Ptr64b; 

#elif defined(GNUC) //GCC

    #ifdef __x86_64
        #define ISA_VAL (ISA_X86_64_VAL) 
    #elif __i686
        #define BITS_32
        #define ISA_VAL (ISA_X86_i686_VAL)
    #elif __aarch64__
        #define ISA_VAL (ISA_AARCH64_VAL)    
    #elif __arm__
        #define ISA_VAL (ISA_ARM32_VAL)
        #define BITS_32
    #else
        #define ISA_VAL (ISA_UNKNOWN_VAL)
        #define UNKNOWN_ARCH 1
    #endif
    #define ENVTYPE (POSIX_GCC + (ISA_VAL*ISA_GROUP_RANGE_SIZE))

    typedef uint64_t Ptr64b; 
#endif // no else, we want it to crash.

#define IS_MSVC(env) ((env >= WINDOWS_MSVC_RANGESTART) && (env < (WINDOWS_MSVC_RANGESTART + BUILDENV_GROUP_RANGE_SIZE)))
#define IS_GCC(env) ((env >= POSIX_GCC_RANGESTART) && (env < (POSIX_GCC_RANGESTART + BUILDENV_GROUP_RANGE_SIZE)))

#define IS_ISA_X86_64(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_X86_64_VAL)
#define IS_ISA_X86_i686(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_X86_i686_VAL)
#define IS_ISA_AARCH64(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_AARCH64_VAL)

#define NOT_IMPLEMENTED_YET(feature_name, envs_data) do{fprintf(stderr, "ERROR:%s is not impemented as compiled! It is probably not supported on configuration/HW. Support: %s\n",feature_name, envs_data);exit(-1);}while(0)


/////
#ifdef __x86_64
#elif __i686
#define BITS_32
#elif __aarch64__
#else
#define UNKNOWN_ARCH 1
#endif
/////

#endif

