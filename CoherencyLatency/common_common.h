#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H

#define WINDOWS_MSVC (1001)
#define POSIX_GCC (2001)

#ifdef _MSC_VER
#define ENVTYPE (WINDOWS_MSVC)
typedef LONG64 Ptr64b; 
#else //For now
#define ENVTYPE (POSIX_GCC)
typedef uint64_t Ptr64b; 
#endif


#endif
