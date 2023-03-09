#ifndef COMMON_TIMER_MSR_H
#define COMMON_TIMER_MSR_H
#if ALTERNATE_TIMER_MSRS > 0
#define COMMON_TIMER_H

//<includes>
#include "../common_common.h"
#if IS_WINDOWS(ENVTYPE)
#include <sys\timeb.h>
#elif IS_GCC_POSIX(ENVTYPE)
#include <sys/time.h>
#endif
#include <stdint.h>
#include "../common_msr.h"
// </includes>


typedef struct timer_structure_t{
#if IS_WINDOWS(ENVTYPE)
    struct timeb start;
    struct timeb end;

#elif IS_GCC_POSIX(ENVTYPE) // Possible to look into the updated POSIX library clock_gettime library which has a higher resolution.
    struct timeval startTv;
    struct timeval endTv;

    struct timezone startTz;
    struct timezone endTz;
#endif 
    MsrValueTrack msr_tracker[ALTERNATE_TIMER_MSRS];
    MsrDescriptor msrd;
} TimerStructure;

bool print_time_normalized = false;

typedef struct timer_result_t{
    int64_t time_dif_ms;
    double result;
    uint64_t msr_results[ALTERNATE_TIMER_MSRS];
    uint64_t iterations;
} TimerResult;

//Usage: "TimerStructure t; common_timer_start(&t);"
void common_timer_start(TimerStructure* timer) {
    for (int i = 0; i < ALTERNATE_TIMER_MSRS; i++){ // TODO: Add cross platform hints to compiler to unroll completely?
        common_msr_get_update_delta(timer->msrd, &(timer->msr_tracker[i]));
    }    
#if IS_WINDOWS(ENVTYPE)
    ftime(&(timer->start));
#elif IS_GCC_POSIX(ENVTYPE)
    gettimeofday(&(timer->startTv),&(timer->startTz));
#endif
} 

void common_timer_end(TimerStructure* timer, TimerResult* results) {
#if IS_WINDOWS(ENVTYPE)
    ftime(&(timer->end));
    results->time_dif_ms = 1000 * (timer->end.time - timer->start.time) + (timer->end.millitm - timer->start.millitm);
#elif IS_GCC_POSIX(ENVTYPE)
    gettimeofday(&(timer->endTv),&(timer->endTz));
    results->time_dif_ms = 1000 * (timer->endTv.tv_sec - timer->startTv.tv_sec) + ((timer->endTv.tv_usec - timer->startTv.tv_usec) / 1000);
#endif
    results->result =  0;
    for (int i = 0; i < ALTERNATE_TIMER_MSRS; i++){ // TODO:  Add cross platform hints to compiler to unroll completely?
        results->msr_results[i] = common_msr_get_update_delta(timer->msrd, &(timer->msr_tracker[i]));
    }   
}

void common_timer_result_process_iterations(TimerResult* results, unsigned int iterations_cnt) {
    if (iterations_cnt != 0)
        results->result = 1e6 * (float)results->time_dif_ms / (float)iterations_cnt;
    else
        results->result =  1e6 * (float)results->time_dif_ms; 
    results->iterations = iterations_cnt;
}

void common_timer_result_process_bw(TimerResult* results, int transferred) {
//    double mbTransferred = ((iterations_cnt == 0 ? 1: iterations_cnt) * bw_per_itr)  / (double)1e6;
    results->result =  1000 * transferred / ((double)(results->time_dif_ms)); 
}

void common_timer_result_fprint(TimerResult* results, FILE* fd) {
//    double mbTransferred = ((iterations_cnt == 0 ? 1: iterations_cnt) * bw_per_itr)  / (double)1e6;
    fprintf(fd, "%f,", results->result); 
    for (int i = 0; i < ALTERNATE_TIMER_MSRS; i++){ // TODO:  Add cross platform hints to compiler to unroll completely?
        if (!print_time_normalized)
            fprintf(fd, "%lld,", results->msr_results[i]); 
        else{
            fprintf(fd, "%f,", results->msr_results[i] /  ((double)(results->time_dif_ms))); 
        }

    }    
    fprintf(fd, "%lld,", results->iterations); 

}




/**
//Usage (given started TimerStructure t and iteration count iter): "TimerResult t_res; common_timer_end(&t, &t_res, iter);"
inline void common_timer_end(TimerStructure* timer, TimerResult* results, unsigned int iterations_cnt) {
#if IS_WINDOWS(ENVTYPE)
    ftime(&(timer->end));
    results->time_dif_ms = 1000 * (timer->end.time - timer->start.time) + (timer->end.millitm - timer->start.millitm);
#elif IS_GCC_POSIX(ENVTYPE)
    gettimeofday(&(timer->endTv),&(timer->endTz));
    results->time_dif_ms = 1000 * (timer->endTv.tv_sec - timer->startTv.tv_sec) + ((timer->endTv.tv_usec - timer->startTv.tv_usec) / 1000);
#endif
    if (iterations_cnt != 0)
        results->per_iter_ns = 1e6 * (float)results->time_dif_ms / (float)iterations_cnt;
    else
        results->per_iter_ns =  1e6 * (float)results->time_dif_ms; 
    results->bw_in_MB = 0;
}
inline void common_timer_end_bw(TimerStructure* timer, TimerResult* results, unsigned int iterations_cnt, int bw_per_itr) {
#if IS_WINDOWS(ENVTYPE)
    ftime(&(timer->end));
    results->time_dif_ms = 1000 * (timer->end.time - timer->start.time) + (timer->end.millitm - timer->start.millitm);
#elif IS_GCC_POSIX(ENVTYPE)
    gettimeofday(&(timer->endTv),&(timer->endTz));
    results->time_dif_ms = 1000 * (timer->endTv.tv_sec - timer->startTv.tv_sec) + ((timer->endTv.tv_usec - timer->startTv.tv_usec) / 1000);
#endif
    if (iterations_cnt != 0)
        results->per_iter_ns = 1e6 * (float)results->time_dif_ms / (float)iterations_cnt;
    else
        results->per_iter_ns =  1e6 * (float)results->time_dif_ms; 
    double mbTransferred = ((iterations_cnt == 0 ? 1: iterations_cnt) * bw_per_itr)  / (double)1e6;
    results->bw_in_MB =  1000 * mbTransferred / ((double)(results->time_dif_ms)); 
}
*/
/*
//Alternatively we could do this:
// then usage would be "TimerStructure t = common_timer_start();"
inline TimerStructure common_timer_start_() {
    TimerStructure timer;
#if IS_WINDOWS(ENVTYPE)
    ftime(&(timer.start));
#elif IS_GCC_POSIX(ENVTYPE)
    gettimeofday(&(timer.startTv),&(timer.startTz));
#endif
    return timer;
} 
// And here usage would be "TimerResult t_res = common_timer_end(&t, iter);" although we could even give up on end times in struct and have it locally, and then get struct by value, not pointer
inline TimerResult common_timer_end_(TimerStructure* timer, unsigned int iterations_cnt) {
    TimerResult results;
#if IS_WINDOWS(ENVTYPE)
    ftime(&(timer->end));
    results.time_dif_ms = 1000 * (timer->end.time - timer->start.time) + (timer->end.millitm - timer->start.millitm);
#elif IS_GCC_POSIX(ENVTYPE)
    gettimeofday(&(timer->endTv),&(timer->endTz));
    results.time_dif_ms = 1000 * (timer->endTv.tv_sec - timer->startTv.tv_sec) + ((timer->endTv.tv_usec - timer->startTv.tv_usec) / 1000);
#endif
    if (iterations_cnt != 0)
        results.per_iter_ns = 1e6 * (float)results.time_dif_ms / (float)iterations_cnt;
    else
        results.per_iter_ns =  1e6 * (float)results.time_dif_ms; 
    return results;
}
Although syntaxically it might be nicer, I worry that 
  1) it adds more work in the loop so it's less efficient, 
  and 
  2) it's different from the other formats we're emulating.
*/

inline TimerResult common_timer_result_difference(TimerResult A, TimerResult B) {
    TimerResult D;
    D.result = A.result - B.result;
    D.time_dif_ms = A.time_dif_ms - B.time_dif_ms;
    return D;
} 


#endif
#endif
