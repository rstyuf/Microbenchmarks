#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H

#define WINDOWS_MSVC (1001)
#define POSIX_GCC (2001)

#ifdef _MSC_VER
#define ENVTYPE (WINDOWS_MSVC)
#else //For now
#define ENVTYPE (POSIX_GCC)
#endif


#endif
