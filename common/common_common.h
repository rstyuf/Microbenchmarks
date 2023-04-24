#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H


//idea: to be able to hold one variable that functions as a sort of decimal equiv
// of bitfields to hold all env type variations (MSVC/GCC, OS, ISA, calling conventions...)
//For instance: XYZW where:
//               X represents build env {1:MSVC, 2:GCC},
//               Y represents ISA {1:x86-64, 2:x86 (i686), 3: ARM Aarch64, 4: other ARM, 0: Unknown}, 
#define ABOVE_BUILDENV_GROUP_RANGE_SIZE (10000)
#define BUILDENV_GROUP_RANGE_SIZE (1000)
#define BUILDENV_WIN_MSVC_VAL (1)
#define BUILDENV_WIN_MINGW_VAL (2)
#define BUILDENV_POSIX_GCC_VAL (3)
#define _BUILDENV(val) (BUILDENV_GROUP_RANGE_SIZE * val)
/*
#define WINDOWS_MSVC_RANGESTART (BUILDENV_GROUP_RANGE_SIZE)
#define WINDOWS_MSVC (WINDOWS_MSVC_RANGESTART + 1)
#define POSIX_GCC_RANGESTART (WINDOWS_MSVC_RANGESTART + BUILDENV_GROUP_RANGE_SIZE)
#define POSIX_GCC (POSIX_GCC_RANGESTART + 1)
*/
#define ISA_GROUP_RANGE_SIZE (100)
#define ISA_X86_64_VAL   (1)
#define ISA_X86_i686_VAL (2)
#define ISA_AARCH64_VAL  (3)
#define ISA_ARM32_VAL    (4)
#define ISA_UNKNOWN_VAL  (0)
#define _ISA(val) (ISA_GROUP_RANGE_SIZE * val)


// Ptr64b definition: generic datatype used to point to things
// Note that the type is signed in Windows MSVC and unsigned in GCC
// Maybe worth aligning GCC to signed int (int64_t)? Does it matter?
#ifdef _MSC_VER

    #include <windows.h>
    typedef LONG64 Ptr64b; 

    #ifdef _M_ARM64 // Hopefully the documentation is right and this doesn't include ARM64EC
        #define ISA_VAL (ISA_AARCH64_VAL)    
        typedef Ptr64b PtrSysb; 
        #define POINTER_SIZE 8
    #elif _M_ARM    // is this even relevant for windows/MSVC?
        #define ISA_VAL (ISA_ARM32_VAL)    
        typedef DWORD32 PtrSysb; 
        #define POINTER_SIZE 4
        #define BITS_32
    #elif _M_X64 
        #define ISA_VAL (ISA_X86_64_VAL) 
        typedef Ptr64b PtrSysb; 
        #define POINTER_SIZE 8  
    #elif _M_IX86
        #define ISA_VAL (ISA_X86_i686_VAL)  
        typedef DWORD32 PtrSysb; 
        #define POINTER_SIZE 4
        #define BITS_32
    #else
        #define ISA_VAL (ISA_UNKNOWN_VAL)    
        typedef Ptr64b PtrSysb; 
        #define POINTER_SIZE 8

    #endif
    #define ENVTYPE (_BUILDENV(BUILDENV_WIN_MSVC_VAL) + _ISA(ISA_VAL))
#elif defined(__MINGW64__) //MinGW
#define _GNU_SOURCE
#include <stdint.h>
#include <windows.h>

    #ifdef __x86_64
        #define ISA_VAL (ISA_X86_64_VAL) 
        typedef uint64_t PtrSysb; 
        #define POINTER_SIZE 8
    #elif __i686
        #define BITS_32
        #define ISA_VAL (ISA_X86_i686_VAL)
        typedef uint32_t PtrSysb; 
        #define POINTER_SIZE 4
    #elif __aarch64__
        #define ISA_VAL (ISA_AARCH64_VAL) 
        typedef uint64_t PtrSysb; 
        #define POINTER_SIZE 8
    #elif __arm__
        #define ISA_VAL (ISA_ARM32_VAL)
        #define BITS_32
        typedef uint32_t PtrSysb; 
        #define POINTER_SIZE 4
    #else
        #define ISA_VAL (ISA_UNKNOWN_VAL)
        #define UNKNOWN_ARCH 1
        typedef uint64_t PtrSysb;  //Default
        #define POINTER_SIZE 8 //default
    #endif
    #define ENVTYPE (_BUILDENV(BUILDENV_WIN_MINGW_VAL) + _ISA(ISA_VAL))

    typedef LONG64 Ptr64b; 
#elif defined(__GNUC__) //GCC
#define _GNU_SOURCE
#include <stdint.h>
    #ifdef __x86_64
        #define ISA_VAL (ISA_X86_64_VAL) 
        typedef uint64_t PtrSysb; 
        #define POINTER_SIZE 8
    #elif __i686
        #define BITS_32
        #define ISA_VAL (ISA_X86_i686_VAL)
        typedef uint32_t PtrSysb; 
        #define POINTER_SIZE 4
    #elif __aarch64__
        #define ISA_VAL (ISA_AARCH64_VAL)    
        typedef uint64_t PtrSysb; 
        #define POINTER_SIZE 8
    #elif __arm__
        #define ISA_VAL (ISA_ARM32_VAL)
        #define BITS_32
        typedef uint32_t PtrSysb; 
        #define POINTER_SIZE 4
    #else
        #define ISA_VAL (ISA_UNKNOWN_VAL)
        #define UNKNOWN_ARCH 1
        typedef uint64_t PtrSysb; // We set as default
        #define POINTER_SIZE 8 // we set as default
    #endif
    #define ENVTYPE (_BUILDENV(BUILDENV_POSIX_GCC_VAL) + _ISA(ISA_VAL))
    typedef uint64_t Ptr64b; 

#else //we want it to crash.
    #error "Build Env is unsupported. Try Using GCC (mingw on Windows) or MSVC."
#endif 

#define IS_MSVC(env) (((env%ABOVE_BUILDENV_GROUP_RANGE_SIZE)/BUILDENV_GROUP_RANGE_SIZE) ==  BUILDENV_WIN_MSVC_VAL)
#define IS_GCC_MINGW(env) (((env%ABOVE_BUILDENV_GROUP_RANGE_SIZE)/BUILDENV_GROUP_RANGE_SIZE) ==  BUILDENV_WIN_MINGW_VAL)
#define IS_GCC_POSIX(env) (((env%ABOVE_BUILDENV_GROUP_RANGE_SIZE)/BUILDENV_GROUP_RANGE_SIZE) ==  BUILDENV_POSIX_GCC_VAL)
#define IS_GCC(env) ((IS_GCC_MINGW(env)) || (IS_GCC_POSIX(env)))
#define IS_WINDOWS(env) ((IS_GCC_MINGW(env)) || (IS_MSVC(env)))

#define IS_ISA_X86_64(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_X86_64_VAL)
#define IS_ISA_X86_i686(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_X86_i686_VAL)
#define IS_ISA_X86(env) (IS_ISA_X86_64(env) || IS_ISA_X86_i686(env))
#define IS_ISA_AARCH64(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_AARCH64_VAL)
#define IS_ISA_ARM32(env) (((env%BUILDENV_GROUP_RANGE_SIZE)/ISA_GROUP_RANGE_SIZE) ==  ISA_ARM32_VAL)

#define IS_64BITS(env) (POINTER_SIZE == 8)
#define IS_32BITS(env) (POINTER_SIZE == 4)

#define NOT_IMPLEMENTED_YET(feature_name, envs_data) do{fprintf(stderr, "ERROR:%s is not implemented as compiled! It is probably not supported on configuration/HW. Support: %s\n",feature_name, envs_data);exit(-1);}while(0)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "common_timer.h"
#include "common_threading.h"
#include "common_memory.h"
#include "common_datalogger.h"

#endif

