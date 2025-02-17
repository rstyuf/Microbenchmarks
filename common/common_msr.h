#ifndef COMMON_MSR_H
#define COMMON_MSR_H
//<includes>
#include "common_common.h"
//#include "common_threading.h"
#if IS_WINDOWS(ENVTYPE)
#include <windows.h>
#include <tchar.h>
#include <intrin.h>

#include <process.h>
#ifdef QUIET_OUTPUT
#define _WIN_MSR_AUTOLOADER_PATH_LOAD "DriverLoader\\DriverLoader.exe load silent"
#define _WIN_MSR_AUTOLOADER_PATH_UNLOAD "DriverLoader\\DriverLoader.exe unload silent"
#else
#define _WIN_MSR_AUTOLOADER_PATH_LOAD "DriverLoader\\DriverLoader.exe load"
#define _WIN_MSR_AUTOLOADER_PATH_UNLOAD "DriverLoader\\DriverLoader.exe unload"
#endif

#elif IS_GCC_POSIX(ENVTYPE)
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#endif
#if IS_MSVC(ENVTYPE)
//#include <immintrin.h>
#elif IS_GCC(ENVTYPE)
#ifndef WEIRDBUG_CPUID_H_DEFINED
#include <cpuid.h>
#define WEIRDBUG_CPUID_H_DEFINED
#endif
#endif

#include <stdint.h>
// </includes>



typedef struct open_msr_descr_t{
    bool valid;//=false;
#if IS_WINDOWS(ENVTYPE)
    HANDLE winring0DriverHandle;//=INVALID_HANDLE_VALUE;
#elif IS_GCC_POSIX(ENVTYPE)
    int *msrFds;
    int numProcs;
#endif
    bool uncore_counters_enabled;
} MsrDescriptor;


#if IS_WINDOWS(ENVTYPE)
MsrDescriptor _MSR_WIN_OpenWinring0Driver();
void _MSR_WIN_CloseWinring0Driver(MsrDescriptor fd);
uint64_t _MSR_WIN_ReadMsr(MsrDescriptor fd, uint32_t index);
uint64_t _MSR_WIN_WriteMsr(MsrDescriptor fd, uint32_t index, uint64_t value);
#elif IS_GCC_POSIX(ENVTYPE)
void _MSR_LINUX_setAffinity(int core);
int _MSR_LINUX_openMsr(int core);
void _MSR_LINUX_detectCpuMaker();
#endif





MsrDescriptor common_msr_open(){
#if IS_WINDOWS(ENVTYPE)
#ifdef _WIN_MSR_AUTOLOADER_PATH_LOAD
    //TODO! Requires some set path?
    system(_WIN_MSR_AUTOLOADER_PATH_LOAD);
#endif

    return _MSR_WIN_OpenWinring0Driver();
#elif IS_GCC_POSIX(ENVTYPE)
    MsrDescriptor msr_d;
    msr_d.numProcs = common_threading_get_num_cpu_cores();
    msr_d.msrFds = (int *)malloc(sizeof(int) * msr_d.numProcs);
    memset(msr_d.msrFds, 0, sizeof(int) * msr_d.numProcs);
    for (int i = 0; i < msr_d.numProcs; i++) {
        //setAffinity(i);
        if (!msr_d.msrFds[i]) msr_d.msrFds[i] = _MSR_LINUX_openMsr(i);
    }
#else
    NOT_IMPLEMENTED_YET("MSR support", "Only implemented for Windows and Linux on x86");
#endif
}

 void common_msr_close(MsrDescriptor fd){
 #if IS_WINDOWS(ENVTYPE)
    _MSR_WIN_CloseWinring0Driver(fd);
#elif IS_GCC_POSIX(ENVTYPE)
    if (!fd.valid) return;
    for (int i = 0; i < fd.numProcs; i++) {
        //setAffinity(i);
        if (fd.msrFds[i]!= 0) close(msr_d.msrFds[i]);
    }
    free(fd.msrFds);
    fd.valid = false;
#else
    NOT_IMPLEMENTED_YET("MSR support", "Only implemented for Windows and Linux on x86");
#endif
}


inline uint64_t common_msr_read(MsrDescriptor fd, uint32_t addr){
#if IS_WINDOWS(ENVTYPE)
    return _MSR_WIN_ReadMsr( fd, addr);
#elif IS_GCC_POSIX(ENVTYPE)
    uint64_t result, bytesRead;
    bytesRead = pread(fd, &result, sizeof(result), addr);
    if (bytesRead != sizeof(result)) {
        fprintf(stderr, "Could not read from fd %d, msr %u\n", fd, addr);
    }
    return result;
#else
    NOT_IMPLEMENTED_YET("MSR support", "Only implemented for Windows and Linux on x86");
#endif
}


inline void common_msr_write(MsrDescriptor fd, uint32_t addr, uint64_t value){
#if IS_WINDOWS(ENVTYPE)
     _MSR_WIN_WriteMsr(fd, addr, value);
     return;
#elif IS_GCC_POSIX(ENVTYPE)
    uint64_t bytesWritten, newValue;
    bytesWritten = pwrite(fd, &value, sizeof(value), addr);
    if (bytesWritten != sizeof(value)) {
        fprintf(stderr, "Could not write to fd %d, msr %u, value %lu\n", fd, addr, value);
    }
#else
    NOT_IMPLEMENTED_YET("MSR support", "Only implemented for Windows and Linux on x86");
#endif
}

typedef enum cpu_vendor_enum{CPU_VENDOR_INTEL, CPU_VENDOR_AMD, CPU_VENDOR_UNKNOWN} CpuVendorEnum;
CpuVendorEnum common_msr_detect_cpu_vendor() {
#if IS_ISA_X86(ENVTYPE)
    uint32_t *uintPtr;
    char cpuName[13];
//I split by IS_MSVC and IS_GCC, but it's possible the correct split is somewhere else.
#if IS_MSVC(ENVTYPE)
    uintPtr = (uint32_t *)cpuName;
    __cpuidex((int*) uintPtr, 0, 0);
#elif IS_GCC(ENVTYPE)
    uint32_t cpuidEax, cpuidEbx, cpuidEcx, cpuidEdx;
    __cpuid_count(0, 0, cpuidEax, cpuidEbx, cpuidEcx, cpuidEdx);
    uintPtr = (uint32_t *)cpuName;
    uintPtr[0] = cpuidEbx;
    uintPtr[1] = cpuidEdx;
    uintPtr[2] = cpuidEcx;
#else
    NOT_IMPLEMENTED_YET("CPU Vendor Detection", "Only implemented for MSVC and GCC");
#endif //IS_MSCV vs IS_GCC
    cpuName[12] = 0;
    fprintf(stderr, "CPU name: %s\n", cpuName);
    if (memcmp(cpuName, "GenuineIntel", 12) == 0) {
        fprintf(stderr, "Looks like Intel\n");
        return CPU_VENDOR_INTEL;
    } else if (memcmp(cpuName, "AuthenticAMD", 12) == 0) {
        fprintf(stderr, "Looks like AMD\n");
        return CPU_VENDOR_AMD;
    }
    else {
        fprintf(stderr, "CPU Vendor seems unknown!\n");
        return CPU_VENDOR_UNKNOWN;
    }
#else /*IS_ISA_X86(ENVTYPE)*/
    NOT_IMPLEMENTED_YET("CPU Vendor Detection", "Only implemented for x86 (both 32 and 64 bit)");
    return CPU_VENDOR_UNKNOWN; 
#endif
}



////////////
#if IS_WINDOWS(ENVTYPE)
MsrDescriptor _MSR_WIN_OpenWinring0Driver()
{
    MsrDescriptor msr_d;
    msr_d.winring0DriverHandle = CreateFile(
        _T("\\\\.\\") _T("WinRing0_1_2_0"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (msr_d.winring0DriverHandle == INVALID_HANDLE_VALUE)
    {  
        msr_d.valid = false;
        int error = GetLastError();
        fprintf(stderr, "Failed to get handle to WinRing0_1_2_0 driver: %d\n", error);
    } 
    else msr_d.valid = true;
    return msr_d;
}

#define _MSR_WIN_OLS_TYPE 40000
#define _MSR_WIN_IOCTL_OLS_READ_MSR CTL_CODE(_MSR_WIN_OLS_TYPE, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define _MSR_WIN_IOCTL_OLS_READ_MSR CTL_CODE(_MSR_WIN_OLS_TYPE, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define _MSR_WIN_IOCTL_OLS_WRITE_MSR CTL_CODE(_MSR_WIN_OLS_TYPE, 0x822, METHOD_BUFFERED, FILE_ANY_ACCESS)
void _MSR_WIN_CloseWinring0Driver(MsrDescriptor fd){
#ifdef _WIN_MSR_AUTOLOADER_PATH_UNLOAD
    //TODO! Requires some set path?
    system(_WIN_MSR_AUTOLOADER_PATH_UNLOAD);
#endif

}

inline uint64_t _MSR_WIN_ReadMsr(MsrDescriptor fd, uint32_t index)
{
    DWORD returnedLength;
    bool result;
    uint64_t retval;
    if (fd.winring0DriverHandle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Invalid handle to Winring0 driver\n");
        return 0;
    }

    result = DeviceIoControl(
        fd.winring0DriverHandle,
        _MSR_WIN_IOCTL_OLS_READ_MSR,
        &index,
        sizeof(index),
        &retval,
        sizeof(retval),
        &returnedLength,
        NULL
    );

    if (!result)
    {
        fprintf(stderr, "DeviceIoControl to read MSR failed\n");
    }

    return retval;
}

#pragma pack(push,4)
typedef struct  _ols_write_msr_input_buf_t {
	uint32_t addr;
	uint64_t value;
} ols_write_msr_input_buf;


inline uint64_t _MSR_WIN_WriteMsr(MsrDescriptor fd, uint32_t index, uint64_t value)
{
    DWORD returnedLength;
    bool result;
    uint64_t retval;
    if (fd.winring0DriverHandle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Invalid handle to Winring0 driver\n");
        return 0;
    }
    ols_write_msr_input_buf write_struct;
    write_struct.addr = index;
    write_struct.value = value;

    result = DeviceIoControl(
        fd.winring0DriverHandle,
        _MSR_WIN_IOCTL_OLS_WRITE_MSR,
        &write_struct,
        sizeof(write_struct),
        &retval,
        sizeof(retval),
        &returnedLength,
        NULL
    );

    if (!result)
    {
        fprintf(stderr, "DeviceIoControl to write MSR failed\n");
    }

    return retval;
}
#elif IS_GCC_POSIX(ENVTYPE)

void _MSR_LINUX_setAffinity(int core) {
    int rc;
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    rc = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
    if (rc != 0) {
        fprintf(stderr, "unable to set thread affinity to %d\n", core);
    }
}
int _MSR_LINUX_openMsr(int core) {
    char msrFilename[255];
    int fd;
    sprintf(msrFilename, "/dev/cpu/%d/msr", core);
    fd = open(msrFilename, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open MSR file, core %d\n", core);
        return -1;
    }
    return fd;
}



#endif
////////////////////////////////////////
typedef struct msr_value_tracker_t{
    uint16_t specific_core; //MAX_INT16 being for uncore or anyways locked core? Not yet implemented.
    uint8_t usage_type; //0 for disabled, 1 for reads, 2 for read and zero? // Unimplemented yet
    uint8_t value_fieldsize_bits; //wrap_around_bits;
    uint32_t addr;
    uint64_t last_value;
} MsrValueTrack;

inline uint64_t common_msr_get_update_delta(MsrDescriptor msrd, MsrValueTrack* tracker){
    if (msrd.valid == false) return -1;
    if (tracker->usage_type == 1){ //read:
        uint64_t newval = common_msr_read(msrd, tracker->addr);
        //printf("ReadMsr: Addr %x got %lld   (oldval %lld) diff: %lld       (0x%llx)\n", tracker->addr, newval, tracker->last_value, newval-tracker->last_value, newval-tracker->last_value);
        // should I seperate the read stage from the processing stage and do all reads first in one go?
        //  and then process in one go? Reasoning: processing time might be significant enough to skew in some edge cases?
        int64_t difference = newval - tracker->last_value;
        if (difference < 0){
            difference += (1<< tracker->value_fieldsize_bits);
        }
        tracker->last_value = newval;
        return difference;
    }
    else if (tracker->usage_type == 2) { // Read and Zero
        uint64_t newval = common_msr_read(msrd, tracker->addr);
        //printf("ReadMsrAndZero: Addr %x got %lld  (0x%llx)   \n", tracker->addr, newval, newval);
        common_msr_write(msrd, tracker->addr, 0);

        // should I seperate the read stage from the processing stage and do all reads first in one go?
        //  and then process in one go? Reasoning: processing time might be significant enough to skew in some edge cases?
        //int64_t difference = newval - tracker->last_value;
        // if (difference < 0){
        //     difference += (1<< tracker->value_fieldsize_bits);
        // }
        // tracker->last_value = newval;
        return newval;

    }
    else{
        return 0;
    }
}

////////////////////////////////////////

/*
uint64_t startEnergy;

void StartMeasuringEnergy()
{
    uint64_t rawEnergy = ReadMsr(INTEL_PKG_ENERGY_STATUS_MSR);
    rawEnergy &= 0xFFFFFFFF;
    startEnergy = rawEnergy;
}

/// <summary>
/// Stops measuring power and gives back consumed energy in joules
/// </summary>
/// <returns>Consumed energy in joules</returns>
float StopMeasuringEnergy()
{
    uint64_t elapsedEnergy;
    uint64_t rawEnergy = ReadMsr(INTEL_PKG_ENERGY_STATUS_MSR);
    rawEnergy &= 0xFFFFFFFF;

    if (rawEnergy < startEnergy)
    {
        uint64_t extraEnergy = 0xFFFFFFFF - startEnergy;
        elapsedEnergy = extraEnergy + rawEnergy;
    }
    else
    {
        elapsedEnergy = rawEnergy - startEnergy;
    }

    return elapsedEnergy * energyStatusUnits;
}
*/
//
#endif /*COMMON_MSR_H*/
