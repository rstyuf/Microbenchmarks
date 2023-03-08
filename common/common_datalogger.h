#ifdef ALTERNATE_DATALOGGER
    #if ALTERNATE_DATALOGGER == 1
    #include "alternative_implementations/commonalt_datalogger_foldercsvs.h"
    #endif
#endif
#ifndef COMMON_DATALOGGER_H
#define COMMON_DATALOGGER_H
//<includes>
#include "common_common.h"
#include "common_timer.h"

typedef struct datalogger_structure_t{
    bool enable;
    /*
     What do I want to log?
      For each entry I want:
      Metadata on results
      Test Data

    */
} DataLog;


void common_datalogger_log_latency(DataLog* dl, TimerResult result, const char* notes, int sizekb){
    
}
void common_datalogger_log_latency_crossnode(DataLog* dl, TimerResult result, const char* notes, int sizekb, int src_node, int dest_node){
    
}
void common_datalogger_log_c2c(DataLog* dl, TimerResult result, const char* notes, int src_core, int dest_core){
    
}
void common_datalogger_log_bandwidth(DataLog* dl, TimerResult result, const char* notes, int sizekb, int threads){
    
}
void common_datalogger_log_bandwidth_mlp(DataLog* dl, TimerResult result, const char* notes, int sizekb, int parallelism){
    
}
void common_datalogger_log_latency_stlf(DataLog* dl, TimerResult result, const char* notes, int store_offset, int load_offset){
    
}
FILE* common_datalogger_log_getfd(DataLog* dl){
    if (dl == NULL)
        return stdout;
    ///
    return stdout;
}


#endif /*COMMON_DATALOGGER_H*/
