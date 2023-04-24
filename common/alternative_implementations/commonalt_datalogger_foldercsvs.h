#ifndef COMMON_DATALOGGER_FOLDERCSV_H
#define COMMON_DATALOGGER_FOLDERCSV_H
#if ALTERNATE_DATALOGGER == 1
#define COMMON_DATALOGGER_H

//<includes>
#include "../common_common.h"
#include "../common_timer.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#if IS_WINDOWS(ENVTYPE)
#include <windows.h>
#define OS_PATH_SEP '\\'
#elif IS_GCC_POSIX(ENVTYPE)
#include <sys/stat.h>
#define OS_PATH_SEP '/'
#endif
//</includes>
#define STRUCT_STRING_MAX (128)
//Please keep names to less than 90 chars and in English without special characters
#define STRUCT_STRING_NAMES_MAX (STRUCT_STRING_MAX - 32)

#define LOGGER_MKDIR_PATH_PERMISSIONS (0777)
#define SAVEKEYROW true

typedef struct datalogger_structure_t{
    bool enable;

    bool enable_logfd_out;
    FILE* log_fd_output;

    char testset_name_ts_folder[STRUCT_STRING_MAX];
    char testset_origname[STRUCT_STRING_MAX];
    char active_subtest_name[STRUCT_STRING_MAX];

    // Not sure these names are necessary to keep track of here 
    /*
    char* log_name_memlatency[STRUCT_STRING_MAX];
    char* log_name_c2c[STRUCT_STRING_MAX];
    char* log_name_membw[STRUCT_STRING_MAX];
    char* log_name_mlp[STRUCT_STRING_MAX];
    char* log_name_stlf[STRUCT_STRING_MAX];
    char* log_name_clocks[STRUCT_STRING_MAX];
    */
    FILE* log_fd_memlatency;
    FILE* log_fd_c2c;
    FILE* log_fd_membw;
    FILE* log_fd_mlp;
    FILE* log_fd_stlf;
    FILE* log_fd_clocks;
    FILE* log_fd_extra_undef;


    char predef_note[STRUCT_STRING_MAX];
    int predef_data1;
    int predef_data2;
    int predef_data3;
    int predef_data4;

    /*
     What do I want to log?
      For each entry I want:
      Metadata on results
      Test Data

    */
} DataLog;

typedef enum DATA_LOGGER_LOG_TYPES{
    DATA_LOGGER_LOG_TYPES_MEMLAT,
    //
    DATA_LOGGER_LOG_TYPES_C2C,
    //
    DATA_LOGGER_LOG_TYPES_MEMBW,
    //
    DATA_LOGGER_LOG_TYPES_MLP,
    //
    DATA_LOGGER_LOG_TYPES_STLF,
    //
    DATA_LOGGER_LOG_TYPES_CLOCKS,
    //
    DATA_LOGGER_LOG_TYPES_EXTRA_UNDEF,

    DATA_LOGGER_LOG_TYPES_OUTPUTFD,

} DataLoggerLogTypes;

bool common_datalogger_init_testset(DataLog* dl, char* testset_name){
    int ret = strlen(testset_name);
    //char time_string[32] = {0}; //21 Bytes should be enough as originally formatted but to be safe.
    if ( ret > STRUCT_STRING_NAMES_MAX){
        printf("Error: Testset name is too long! Max len: %d. Testset name is %d\n Stopping... \n", STRUCT_STRING_NAMES_MAX, ret);
        return false;
    }
    time_t t = time(NULL);
    
    struct tm time_struct;
    #if IS_WINDOWS(ENVTYPE)
    localtime_s(&time_struct, &t); //Because I guess Microsoft thinks consistency with standards is for others but not them?
    #elif IS_GCC_POSIX(ENVTYPE)
    localtime_r(&t, &time_struct); 
    #else
        NOT_IMPLEMENTED_YET("DataLogger (timer data)", "Implemented on Windows and on GCC POSIX");
    #endif

    //snprintf(time_string,22, "%04d-%02d-%02d-%02d-%02d-%02d", time_struct.tm_year + 1900 , time_struct.tm_mon +1 , time_struct.tm_mday , time_struct.tm_hour, time_struct.tm_min , time_struct.tm_sec );
    ret = snprintf(dl->testset_name_ts_folder, STRUCT_STRING_MAX, "%s--%04d-%02d-%02d-%02d-%02d-%02d", testset_name, 
             time_struct.tm_year + 1900 , time_struct.tm_mon +1 , time_struct.tm_mday, 
             time_struct.tm_hour, time_struct.tm_min , time_struct.tm_sec );
    
    if (ret <= 0 ){
        printf("Error: Testset folder name generation failure. Returned: %d\n Stopping... \n", ret);
        return false;
    }

    snprintf(dl->testset_origname, STRUCT_STRING_MAX, "%s", testset_name); // can replace with strncpy but I already had this format open
    // Honestly, not sure if I care if this failed.

    #if IS_WINDOWS(ENVTYPE)
    ret = CreateDirectoryA(dl->testset_name_ts_folder, NULL);
    if (ret == 0 && GetLastError() == ERROR_ALREADY_EXISTS){
        printf("Error: Testset folder already exists somehow. Ret: %d\n Stopping... \n", ret);
        return false;
    }
    if (ret == 0 && GetLastError() == ERROR_PATH_NOT_FOUND){
        printf("Error: Testset folder creation got ERROR_PATH_NOT_FOUND. Ret: %d\n Stopping... \n", ret);
        return false;
    }
    #elif IS_GCC_POSIX(ENVTYPE)
    ret = mkdir(dl->testset_name_ts_folder, LOGGER_MKDIR_PATH_PERMISSIONS);
    if (ret < 0){
        perror("Error: Testset folder creation failed. Stopping...\n Error:");
        return false;
    }
    #else
        NOT_IMPLEMENTED_YET("DataLogger (mkdir)", "Implemented on Windows and on GCC POSIX");
    #endif
}


FILE** _common_datalogger_get_fd_for_type(DataLog* dl, DataLoggerLogTypes logtype){
    FILE** fd;
    switch(logtype){
    case DATA_LOGGER_LOG_TYPES_MEMLAT:
        fd = &(dl->log_fd_memlatency);
        break;
    case DATA_LOGGER_LOG_TYPES_C2C:
        fd = &(dl->log_fd_c2c);
        break;
    case DATA_LOGGER_LOG_TYPES_MEMBW:
        fd = &(dl->log_fd_membw);
        break;
    case DATA_LOGGER_LOG_TYPES_MLP:
        fd = &(dl->log_fd_mlp);
        break;
    case DATA_LOGGER_LOG_TYPES_STLF:
        fd = &(dl->log_fd_stlf);
        break;
    case DATA_LOGGER_LOG_TYPES_CLOCKS:
        fd = &(dl->log_fd_clocks);
        break;
    case DATA_LOGGER_LOG_TYPES_OUTPUTFD:
        fd = &(dl->log_fd_output);
        break;
    default:
        fd = &(dl->log_fd_extra_undef);
    }
    return fd;
}

bool common_datalogger_init_test_file(DataLog* dl, DataLoggerLogTypes logtype, char* testfile_name_in){
    char testfile_pathname[2*STRUCT_STRING_NAMES_MAX+1] = {0};
    char test_filename[STRUCT_STRING_NAMES_MAX] = {0};
    int ret;

    if (testfile_name_in != NULL){
        ret = snprintf(test_filename, STRUCT_STRING_MAX, "%s", testfile_name_in);
        if (ret <= 0 ){
            printf("Error: Testset file name generation (1) failure. Returned: %d\n Stopping... \n", ret);
            return false;
        }
    }
    else { // testfile_name_in == NULL
        switch(logtype){
            case DATA_LOGGER_LOG_TYPES_MEMLAT:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryLatency.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_C2C:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "CoreToCoreLatency.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_MEMBW:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryBandwidth.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_MLP:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryLtMLP.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_STLF:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryStoreLoadFwd.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_CLOCKS:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "Clocks.csv");
                break;
            default:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "UndefinedTest.csv");          
        }
        
        if (ret <= 0 ){
            printf("Error: Testset file name generation (2-On log %d) failure. Returned: %d\n Stopping... \n", logtype, ret);
            return false;
        }
    }

    ret = snprintf(testfile_pathname, 2*STRUCT_STRING_MAX, "%s%c%s", dl->testset_name_ts_folder, OS_PATH_SEP, test_filename);
    if (ret <= 0 ){
        printf("Error: Testset file name generation (3) failure. Returned: %d\n Stopping... \n", ret);
        return false;
    }
    // Now lets open the file with create
    FILE** fd = _common_datalogger_get_fd_for_type(dl, logtype);
    /*
    switch(logtype){
    case DATA_LOGGER_LOG_TYPES_MEMLAT:
        fd = &(dl->log_fd_memlatency);
        break;
    case DATA_LOGGER_LOG_TYPES_C2C:
        fd = &(dl->log_fd_c2c);
        break;
    case DATA_LOGGER_LOG_TYPES_MEMBW:
        fd = &(dl->log_fd_membw);
        break;
    case DATA_LOGGER_LOG_TYPES_MLP:
        fd = &(dl->log_fd_mlp);
        break;
    case DATA_LOGGER_LOG_TYPES_STLF:
        fd = &(dl->log_fd_stlf);
        break;
    case DATA_LOGGER_LOG_TYPES_CLOCKS:
        fd = &(dl->log_fd_clocks);
        break;
    case DATA_LOGGER_LOG_TYPES_OUTPUTFD:
        fd = &(dl->log_fd_output);
        break;
    default:
        fd = &(dl->log_fd_extra_undef);*/

    *fd = fopen(testfile_pathname, "w");
    if (*fd == NULL){
        printf("Error: Testset file creation failure for file %s. Errno: %d\n Stopping... \n", testfile_pathname,  errno);
        return false;
    }
    return true;
}



bool common_datalogger_init_all(DataLog* dl, char* testset_name){
    bool ret;
    ret = common_datalogger_init_testset(dl, testset_name);
    if (!ret) return ret;
    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_OUTPUTFD, "outfd.txt");
    if (!ret) return ret;

    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_MEMLAT, NULL);
    if (!ret) return ret;

    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_C2C, NULL);
    if (!ret) return ret;

    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_MEMBW, NULL);
    if (!ret) return ret;

    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_MLP, NULL);
    if (!ret) return ret;

    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_STLF, NULL);
    if (!ret) return ret;

    ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_CLOCKS, NULL);
    if (!ret) return ret;

    return true;
}

void common_datalogger_close_all(DataLog* dl){
    fclose(dl->log_fd_output);
    fclose(dl->log_fd_memlatency);
    fclose(dl->log_fd_c2c);
    fclose(dl->log_fd_membw);
    fclose(dl->log_fd_mlp);
    fclose(dl->log_fd_stlf);
    fclose(dl->log_fd_clocks);
    fclose(dl->log_fd_extra_undef);
}

bool common_datalogger_swap_outfd(DataLog* dl, char* new_outfd_name, bool enabled){ // Note that if the same name is reused it will overwrite!
    fclose(dl->log_fd_output);
    if (enabled){
        bool ret = common_datalogger_init_test_file(dl,  DATA_LOGGER_LOG_TYPES_OUTPUTFD, new_outfd_name);
        if (!ret) return ret;
        dl->enable_logfd_out = true;
    } else {
        dl->enable_logfd_out = false;
    }
    return true;
}

//void common_datalogger_init_subtest(DataLog* dl, DataLoggerLogTypes logtype, char* subtest_name, bool incl_predef_note, int incl_predef_data_upto, char* test_name, int msrs_logged_cnt, char** msrs_logged_names){
void common_datalogger_init_subtest(DataLog* dl, DataLoggerLogTypes logtype, char* subtest_name){
    FILE** fd = _common_datalogger_get_fd_for_type(dl, logtype);
    dl->enable = true;
    int ret;
    fprintf(*fd, "//<<Test,%s,<parallelism>,<src>,<dst>,<sizekb>, <cleanResults>,", subtest_name);
    #ifdef ALTERNATE_TIMER_MSRS
    for (int i = 0; i < ALTERNATE_TIMER_MSRS; i++){
        fprintf(*fd, "<msr %d raw>,", i);
    }
    fprintf(*fd, "<iterations>,");
    #endif
    fprintf(*fd, ",<notes>, <predefnote>, <predef1>, <predef2>, <predef3>, <predef4>,,,\n");
    ret = snprintf(dl->active_subtest_name, STRUCT_STRING_MAX, "%s", subtest_name);
    if (ret <= 0 ){
        printf("Error: Testset file name generation (1) failure. Returned: %d\n  \n", ret);
        //return false;
    }
/*
    #if SAVEKEYROW
        switch(logtype){
            case DATA_LOGGER_LOG_TYPES_MEMLAT:
                fprintf(*fd, "//, , ,\n", subtest_name);
                break;
            case DATA_LOGGER_LOG_TYPES_C2C:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "CoreToCoreLatency.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_MEMBW:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryBandwidth.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_MLP:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryLtMLP.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_STLF:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "MemoryStoreLoadFwd.csv");
                break;
            case DATA_LOGGER_LOG_TYPES_CLOCKS:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "Clocks.csv");
                break;
            default:
                ret = snprintf(test_filename, STRUCT_STRING_MAX, "UndefinedTest.csv");          
        }
    #endif
*/
}


void common_datalogger_log_latency(DataLog* dl, TimerResult result, const char* notes, int sizekb){
    fprintf(dl->log_fd_memlatency, ",%s,, , , %d,", dl->active_subtest_name, sizekb);
    //Print result
    common_timer_result_fprint(&result, dl->log_fd_memlatency);
    fprintf(dl->log_fd_memlatency, ",%s, %s,%d,%d,%d,%d,\n", (notes == NULL? "": notes), dl->predef_note, dl->predef_data1,dl->predef_data2,dl->predef_data3,dl->predef_data4);
}

void common_datalogger_log_latency_crossnode(DataLog* dl, TimerResult result, const char* notes, int sizekb, int src_node, int dest_node){
    fprintf(dl->log_fd_memlatency, ",%s,, %d, %d, %d,", dl->active_subtest_name, src_node, dest_node, sizekb);
    //Print result
    common_timer_result_fprint(&result, dl->log_fd_memlatency);
    fprintf(dl->log_fd_memlatency, ",%s, %s,%d,%d,%d,%d,\n", (notes == NULL? "": notes),dl->predef_note, dl->predef_data1,dl->predef_data2,dl->predef_data3,dl->predef_data4);
}

void common_datalogger_log_c2c(DataLog* dl, TimerResult result, const char* notes, int src_core, int dest_core){
    fprintf(dl->log_fd_c2c, ",%s,, %d, %d, ,", dl->active_subtest_name, src_core, dest_core);
    //Print result
    common_timer_result_fprint(&result, dl->log_fd_c2c);
    fprintf(dl->log_fd_c2c, ",%s, %s,%d,%d,%d,%d,\n", (notes == NULL? "": notes),dl->predef_note, dl->predef_data1,dl->predef_data2,dl->predef_data3,dl->predef_data4);    
}

void common_datalogger_log_bandwidth(DataLog* dl, TimerResult result, const char* notes, int sizekb, int threads){
    fprintf(dl->log_fd_membw, ",%s, %d, , , %d,", dl->active_subtest_name, threads, sizekb);
    //Print result
    common_timer_result_fprint(&result, dl->log_fd_membw);
    fprintf(dl->log_fd_membw, ",%s, %s,%d,%d,%d,%d,\n", (notes == NULL? "": notes),dl->predef_note, dl->predef_data1,dl->predef_data2,dl->predef_data3,dl->predef_data4);    

}
void common_datalogger_log_bandwidth_mlp(DataLog* dl, TimerResult result, const char* notes, int sizekb, int parallelism){
    fprintf(dl->log_fd_mlp, ",%s, %d, , , %d,", dl->active_subtest_name, parallelism, sizekb);
    //Print result
    common_timer_result_fprint(&result, dl->log_fd_mlp);
    fprintf(dl->log_fd_mlp, ",%s, %s,%d,%d,%d,%d,\n", (notes == NULL? "": notes),dl->predef_note, dl->predef_data1,dl->predef_data2,dl->predef_data3,dl->predef_data4);   
}

void common_datalogger_log_latency_stlf(DataLog* dl, TimerResult result, const char* notes, int store_offset, int load_offset){
    fprintf(dl->log_fd_stlf, ",%s, , %d, %d, ,", dl->active_subtest_name, store_offset, load_offset);
    //Print result
    common_timer_result_fprint(&result, dl->log_fd_stlf);
    fprintf(dl->log_fd_stlf, ",%s, %s,%d,%d,%d,%d,\n", (notes == NULL? "": notes), dl->predef_note, dl->predef_data1,dl->predef_data2,dl->predef_data3,dl->predef_data4);   
}

FILE* common_datalogger_log_getfd(DataLog* dl){
    if (dl == NULL || !(dl->enable_logfd_out))
        return stdout;
    ///
    return dl->log_fd_output;
}   

char* common_datalogger_get_testset_folder_name(DataLog* dl){
    return dl->testset_name_ts_folder;
}
void common_datalogger_set_predef(DataLog* dl, int predef, int indx){
    if (indx == 1){
        dl->predef_data1 = predef;
    } else if (indx == 2){
        dl->predef_data2 = predef;
    } else if (indx == 3){
        dl->predef_data3 = predef;
    } else if (indx == 4){
        dl->predef_data4 = predef;
    } 
}


#endif /*ALTERNATE_DATALOGGER == 1*/
#endif /*COMMON_DATALOGGER_FOLDERCSV_H*/
