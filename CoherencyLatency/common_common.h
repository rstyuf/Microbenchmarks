#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H

#define WINDOWS_MSVC (1001)
#define POSIX_GCC (2001)

// Ptr64b definition: generic datatype used to point to things
// Note that the type is signed in Windows MSVC and unsigned in GCC
// Maybe worth aligning GCC to signed int (int64_t)? Does it matter?
#ifdef _MSC_VER
#define ENVTYPE (WINDOWS_MSVC)
#include <windows.h>
typedef LONG64 Ptr64b; 
#else //For now
#define ENVTYPE (POSIX_GCC)
typedef uint64_t Ptr64b; 
#endif


#endif
