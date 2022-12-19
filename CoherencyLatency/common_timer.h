#ifndef COMMON_TIMER_H
#define COMMON_TIMER_H

//<includes>
#include "common_common.h"
#if ENVTYPE == WINDOWS_MSVC
#include <sys\timeb.h>
#elif ENVTYPE == POSIX_GCC 
#include <sys/time.h>
#endif
#include <stdint.h>
// </includes>
typedef struct timer_structure_t{
#if ENVTYPE == WINDOWS_MSVC
    struct timeb start;
    struct timeb end;

#elif ENVTYPE == POSIX_GCC // Possible to look into the updated POSIX library clock_gettime library which has a higher resolution.
    struct timeval startTv;
    struct timeval endTv;

    struct timezone startTz;
    struct timezone endTz;
#endif
} TimerStructure;


typedef struct timer_result_t{
    int64_t time_dif_ms;
    float per_iter_ns;
} TimerResult;

/*
typedef struct timer_result_t{
#if ENVTYPE == WINDOWS_MSVC

#elif ENVTYPE == POSIX_GCC

#endif
} TimerResult;
*/

void common_timer_start(TimerStructure* timer) {
#if ENVTYPE == WINDOWS_MSVC
    ftime(&(timer->start));
#elif ENVTYPE == POSIX_GCC
    gettimeofday(&(timer->startTv),&(timer->startTz));
#endif
}
void common_timer_end(TimerStructure* timer, TimerResult* results, int iterations_cnt) {
#if ENVTYPE == WINDOWS_MSVC
    ftime(&(timer->end));
    results->time_dif_ms = 1000 * (timer->end.time - timer->start.time) + (timer->end.millitm - timer->start.millitm);
#elif ENVTYPE == POSIX_GCC
    gettimeofday(&(timer->endTv),&(timer->endTz));
    results->time_dif_ms = 1000 * (timer->endTv.tv_sec - timer->startTv.tv_sec) + ((timer->endTv.tv_usec - timer->startTv.tv_usec) / 1000);
#endif
    if (iterations_cnt != 0)
        results->per_iter_ns = 1e6 * (float)results->time_dif_ms / (float)iterations_cnt;
    else
        results->per_iter_ns =  1e6 * (float)results->time_dif_ms; 
}


#endif
