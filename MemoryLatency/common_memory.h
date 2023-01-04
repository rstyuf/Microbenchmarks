#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H
//<includes>
#include "common_common.h"
#if IS_WINDOWS(ENVTYPE)
#include <windows.h>
#elif IS_GCC_POSIX(ENVTYPE)
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#ifdef NUMA
#include <numa.h>
#include <numaif.h>
#endif /*NUMA*/
#endif
#include <stdint.h>

// </includes>

// I don't like this but it's the best way I have to do this without forcing huge crazy changes.
struct allocation_record_t {
    Ptr64b* ptr;
    int type; 
    size_t len; //in bytes
    size_t req_alignment; 
    bool req_hugepage; 
};
#define ALLOC_TYPE_UNUSED (0)
#define ALLOC_TYPE_POSIX_MEMALIGN (1)
#define ALLOC_TYPE_MMAP (2)
#define ALLOC_TYPE_NUMA_ALLOC_ONNODE (3)
#define ALLOC_TYPE_WIN_ALIGNED_ALLOC (4)
#define ALLOC_TYPE_WIN_VIRTUAL_ALLOC (5)
#define ALLOC_TYPE_WIN_VIRTUAL_EX_ALLOC (6)

#define MAX_NUMA_NODES (64)

#define MAX_ALLOCATIONS_PER_RUN (128)
struct allocation_record_t allocations[MAX_ALLOCATIONS_PER_RUN] = {0};

#define COMMON_MEM_ALLOC_NUMA_DISABLED (-1) // Or actually anything negative

Ptr64b* common_mem_malloc_aligned(size_t size_to_alloc, size_t alignment){
    #if IS_WINDOWS(ENVTYPE)
    return (Ptr64b*)_aligned_malloc(size_to_alloc, alignment); 
    #elif IS_GCC_POSIX(ENVTYPE)
    Ptr64b* target;
    if (0 != posix_memalign((void **)(&target), size_to_alloc, alignment)) {
        return NULL;
    } else {
        return target;
    }
    #endif
}
void common_mem_aligned_free(Ptr64b* ptr){
    #if IS_WINDOWS(ENVTYPE)
    _aligned_free(ptr); 
    #elif IS_GCC_POSIX(ENVTYPE)
    free(ptr); 
    #endif
}

Ptr64b* common_mem_malloc_special(size_t size_to_alloc, size_t alignment, bool attempt_hugepage, int numa_memory_node){ // numa_memory_node<0 disables any numa specialization.
    int allocation_record_index = 0;
    for (; allocation_record_index < MAX_ALLOCATIONS_PER_RUN; allocation_record_index){
        if (allocations[allocation_record_index].type == ALLOC_TYPE_UNUSED) break;
    }
    if (allocation_record_index == MAX_ALLOCATIONS_PER_RUN){
        fprintf(stderr, "Out of Allocation Record slots! Can not allocate again :-(\n");
        return NULL;
    }
    #if IS_WINDOWS(ENVTYPE)

    DWORD allocationType = MEM_RESERVE | MEM_COMMIT;
    if (attempt_hugepage) {
        allocationType |= MEM_LARGE_PAGES;
        size_t hugePageSize = 1 << 21;
        size_to_alloc = (((size_to_alloc - 1) / hugePageSize) + 1) * hugePageSize; // rounded up to nearest hugepage
    }
    Ptr64b* alloc_ptr;
    if (!attempt_hugepage && numa_memory_node < 0) {
        alloc_ptr = (Ptr64b*)_aligned_malloc(size_to_alloc, alignment);
        if (alloc_ptr  == NULL) return alloc_ptr;
        allocations[allocation_record_index].ptr = alloc_ptr; 
        allocations[allocation_record_index].type = ALLOC_TYPE_WIN_ALIGNED_ALLOC;
        allocations[allocation_record_index].len = size_to_alloc;
        allocations[allocation_record_index].req_alignment = alignment;
        allocations[allocation_record_index].req_hugepage = attempt_hugepage;
        return alloc_ptr;
    }
    if (numa_memory_node >= 0) {
        alloc_ptr = (Ptr64b*) VirtualAllocExNuma(GetCurrentProcess(),
                                                 NULL,
                                                 size_to_alloc,
                                                 allocationType,
                                                 PAGE_READWRITE,
                                                 numa_memory_node);
        if (alloc_ptr == NULL)
            {
                fprintf(stderr, "Failed to get memory via VirtualExAlloc: %d\n", GetLastError());
                return alloc_ptr;
            }
        allocations[allocation_record_index].ptr = alloc_ptr; 
        allocations[allocation_record_index].type = ALLOC_TYPE_WIN_VIRTUAL_EX_ALLOC;
        allocations[allocation_record_index].len = size_to_alloc;
        allocations[allocation_record_index].req_alignment = alignment;
        allocations[allocation_record_index].req_hugepage = attempt_hugepage;
        return alloc_ptr;
    } else {
        alloc_ptr = (Ptr64b*) VirtualAlloc(NULL,
                                            size_to_alloc,
                                            allocationType, 
                                            PAGE_READWRITE);
        if (alloc_ptr == NULL)
        {
            fprintf(stderr, "Failed to get memory via VirtualAlloc: %d\n", GetLastError());
            return alloc_ptr;
        }
        allocations[allocation_record_index].ptr = alloc_ptr; 
        allocations[allocation_record_index].type = ALLOC_TYPE_WIN_VIRTUAL_ALLOC;
        allocations[allocation_record_index].len = size_to_alloc;
        allocations[allocation_record_index].req_alignment = alignment;
        allocations[allocation_record_index].req_hugepage = attempt_hugepage;
        return alloc_ptr;
    }
    #elif IS_GCC_POSIX(ENVTYPE)

    size_t hugePageSize = 1 << 21;
    Ptr64b* alloc_ptr;
    if (!attempt_hugepage && numa_memory_node < 0) {
        if (0 != posix_memalign((void **)(&alloc_ptr), alignment, size_to_alloc)) {
            fprintf(stderr, "Failed to allocate aligned memory\n");
            alloc_ptr = NULL;
        } else {
            allocations[allocation_record_index].ptr = alloc_ptr; 
            allocations[allocation_record_index].type = ALLOC_TYPE_POSIX_MEMALIGN;
            allocations[allocation_record_index].len = size_to_alloc;
            allocations[allocation_record_index].req_alignment = alignment;
            allocations[allocation_record_index].req_hugepage = attempt_hugepage;
        }
        return alloc_ptr;
    }
    size_t alloc_size = (((size_to_alloc - 1) / hugePageSize) + 1) * hugePageSize; // rounded up to nearest hugepage
    if (attempt_hugepage){
        fprintf(stderr, "mmap-ing %lu bytes\n", alloc_size);
        alloc_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if (alloc_ptr == (void *)-1) { // on failure, mmap will return MAP_FAILED, or (void *)-1
            fprintf(stderr, "Failed to mmap huge pages, errno %d = %s\n", errno, strerror(errno));
            if (numa_memory_node < 0){
                fprintf(stderr, "Will try to use madvise\n");
                if (0 != posix_memalign((void **)(&alloc_ptr), hugePageSize, alloc_size)) {
                    fprintf(stderr, "Failed to allocate 2 MB aligned memory, will not use hugepages\n");
                    alloc_ptr = NULL;
                    return alloc_ptr;
                } else {
                    allocations[allocation_record_index].ptr = alloc_ptr; 
                    allocations[allocation_record_index].type = ALLOC_TYPE_POSIX_MEMALIGN;
                    allocations[allocation_record_index].len = alloc_size;
                    allocations[allocation_record_index].req_alignment = alignment;
                    allocations[allocation_record_index].req_hugepage = attempt_hugepage;
                }
                madvise(alloc_ptr, alloc_size, MADV_HUGEPAGE);
            }
        } else {
            allocations[allocation_record_index].ptr = alloc_ptr; 
            allocations[allocation_record_index].type = ALLOC_TYPE_MMAP;
            allocations[allocation_record_index].len = alloc_size;
            allocations[allocation_record_index].req_alignment = alignment;
            allocations[allocation_record_index].req_hugepage = attempt_hugepage;
        }
    }
    if (numa_memory_node >= 0 && allocations[allocation_record_index].type == ALLOC_TYPE_MMAP){
        uint64_t nodeMask = 1UL << numa_memory_node;
        fprintf(stderr, "mbind-ing pre-allocated arr, size %lu bytes\n", alloc_size);
        long mbind_rc = mbind(alloc_ptr, alloc_size, MPOL_BIND, &nodeMask, 64, MPOL_MF_STRICT | MPOL_MF_MOVE);
        fprintf(stderr, "mbind returned %ld\n", mbind_rc);
        if (mbind_rc != 0) {
            fprintf(stderr, "errno: %d\n", errno);
            return alloc_ptr;
        }
    } else if (numa_memory_node >= 0){
        alloc_ptr = numa_alloc_onnode(alloc_size, numa_memory_node);
        if (alloc_ptr == NULL) {
            fprintf(stderr, "Allocationg NUMA alloc onnode failed!\n");
            return NULL;
        }
        allocations[allocation_record_index].ptr = alloc_ptr; 
        allocations[allocation_record_index].type = ALLOC_TYPE_NUMA_ALLOC_ONNODE;
        allocations[allocation_record_index].len = alloc_size;
        allocations[allocation_record_index].req_alignment = alignment;
        allocations[allocation_record_index].req_hugepage = attempt_hugepage;

        madvise(alloc_ptr, alloc_size, MADV_HUGEPAGE); //TODO: Check: should this be only if hugepages?
    }

    return alloc_ptr;

    #else
        NOT_IMPLEMENTED_YET("Special Allocation Modes not supported!\n", "Only supported on Windows and Linux");
        //fprintf(stderr, "");
        return 0; // 
    #endif
}
void common_mem_special_free(Ptr64b* ptr){
    int allocation_record_index = 0;
    for (; allocation_record_index < MAX_ALLOCATIONS_PER_RUN; allocation_record_index){
        if (allocations[allocation_record_index].ptr == ptr) break;
    }
    if (allocation_record_index == MAX_ALLOCATIONS_PER_RUN){
        fprintf(stderr, "Allocation Record not found, can not free!\n");
        return;
    }
    if (allocations[allocation_record_index].type == ALLOC_TYPE_POSIX_MEMALIGN) {
      #if IS_GCC_POSIX(ENVTYPE)
        free(ptr); 
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "POSIX");
        return; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_MMAP) {
      #if IS_GCC_POSIX(ENVTYPE)
        if (munmap(ptr, allocations[allocation_record_index].len) != 0){
            fprintf(stderr, "Failed to munmap: errno %d = %s\n", errno, strerror(errno));
            return; // we don't free the entry
        } 
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "POSIX");
        return; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_NUMA_ALLOC_ONNODE) {
      #if IS_GCC_POSIX(ENVTYPE)
        #ifdef NUMA
            numa_free(ptr, allocations[allocation_record_index].len); 
        #else
            NOT_IMPLEMENTED_YET("NUMA Free requested on a non NUMA version of the code. Weird Bug!\n", "Linux (numa_free)" );
            return; // 
        #endif /*NUMA*/
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "POSIX");
        return; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_WIN_ALIGNED_ALLOC) {
      #if IS_WINDOWS(ENVTYPE)
        _aligned_free(ptr); 
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "WINDOWS");
        return; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_WIN_VIRTUAL_ALLOC) {
      #if IS_WINDOWS(ENVTYPE)
        VirtualFree(ptr, 0, MEM_RELEASE); 
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "Windows (VirtualFree)" );
        return; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_WIN_VIRTUAL_EX_ALLOC) {
       #if IS_WINDOWS(ENVTYPE)
        VirtualFreeEx(GetCurrentProcess(), ptr, 0, MEM_RELEASE); 
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "Windows (VirtualFreeEx)" );
        return; // 
      #endif
    }
    allocations[allocation_record_index].ptr = NULL;
    allocations[allocation_record_index].type = ALLOC_TYPE_UNUSED;
    allocations[allocation_record_index].len = 0;
    allocations[allocation_record_index].req_alignment = 0;
    allocations[allocation_record_index].req_hugepage = false;
}

Ptr64b* common_mem_numa_move_mem_node(Ptr64b* ptr, int target_numa_memory_node){
#ifdef NUMA
    if (target_numa_memory_node < 0 || target_numa_memory_node >= MAX_NUMA_NODES) {
        fprintf(stderr, "Invalid NUMA Memory node %d . Value must be non-negative and less than MAX_NUMA_NODES (currently 64)\n");
        return NULL;
    }
    int allocation_record_index = 0;
    for (; allocation_record_index < MAX_ALLOCATIONS_PER_RUN; allocation_record_index){
        if (allocations[allocation_record_index].ptr == ptr) break;
    }
    if (allocation_record_index == MAX_ALLOCATIONS_PER_RUN){
        fprintf(stderr, "Allocation Record not found, can not do mem node move!\n");
        return NULL;
    }
        
    if (allocations[allocation_record_index].type == ALLOC_TYPE_POSIX_MEMALIGN) { // todo
      #if IS_GCC_POSIX(ENVTYPE)
        // We could free and realloc? Todo? Or do we want to catch if we do something stupid
        NOT_IMPLEMENTED_YET("Allocation type can not be numa mem node moved! \n", "None, this is wrong allocation type");
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "POSIX");
        return NULL; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_MMAP) {// todo
      #if IS_GCC_POSIX(ENVTYPE)
        uint64_t nodeMask = 1UL << target_numa_memory_node;
        fprintf(stderr, "mbind-ing pointer to move to mem node %d, size %lu bytes\n", target_numa_memory_node, allocations[allocation_record_index].len);
        long mbind_rc = mbind(  ptr, 
                                allocations[allocation_record_index].len, 
                                MPOL_BIND, 
                                &nodeMask, 
                                MAX_NUMA_NODES, 
                                MPOL_MF_STRICT | MPOL_MF_MOVE);
        fprintf(stderr, "mbind returned %ld\n", mbind_rc);
        if (mbind_rc != 0) {
            fprintf(stderr, "errno: %d\n", errno);
        return ptr;
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "POSIX");
        return NULL; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_NUMA_ALLOC_ONNODE) {
        //We reallocate on the right mem node.
      #if IS_GCC_POSIX(ENVTYPE)
            numa_free(ptr, allocations[allocation_record_index].len); 
            ptr = numa_alloc_onnode(allocations[allocation_record_index].len, target_numa_memory_node);
            if (ptr == NULL) {
                fprintf(stderr, "Allocationg NUMA alloc onnode failed!\n");
                return NULL;
            }
            allocations[allocation_record_index].ptr = ptr; 

            if (allocations[allocation_record_index].req_hugepage) //TODO: Check: should this be only if hugepages?
                madvise(ptr, allocations[allocation_record_index].len, MADV_HUGEPAGE); 
            return ptr;
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "POSIX");
        return NULL; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_WIN_ALIGNED_ALLOC) {// todo
      #if IS_WINDOWS(ENVTYPE)
        // We could free and realloc? Todo? Or do we want to catch if we do something stupid
        NOT_IMPLEMENTED_YET("Allocation type can not be numa mem node moved! \n", "None, this is wrong allocation type");
        return NULL;
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "WINDOWS");
        return NULL; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_WIN_VIRTUAL_ALLOC) {// todo
      #if IS_WINDOWS(ENVTYPE)
        // We could free and realloc? Todo? Or do we want to catch if we do something stupid
        NOT_IMPLEMENTED_YET("Allocation type can not be numa mem node moved! \n", "None, this is wrong allocation type");
        return NULL;
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "Windows (VirtualFree)" );
        return NULL; // 
      #endif
    } else if (allocations[allocation_record_index].type == ALLOC_TYPE_WIN_VIRTUAL_EX_ALLOC) {// todo
       #if IS_WINDOWS(ENVTYPE)
        //We reallocate on the right mem node.
        VirtualFreeEx(GetCurrentProcess(), ptr, 0, MEM_RELEASE); 
        DWORD allocationType = MEM_RESERVE | MEM_COMMIT;
        if (allocations[allocation_record_index].req_hugepage) {
            allocationType |= MEM_LARGE_PAGES;
        }
        ptr = (Ptr64b*) VirtualAllocExNuma(GetCurrentProcess(),
                                                 NULL,
                                                 allocations[allocation_record_index].len,
                                                 allocationType,
                                                 PAGE_READWRITE,
                                                 target_numa_memory_node);
        if (ptr == NULL)
            {
                fprintf(stderr, "Failed to get memory via VirtualExAlloc: %d\n", GetLastError());
                return ptr;
            }
        allocations[allocation_record_index].ptr = ptr; 
        return ptr;
      #else
        NOT_IMPLEMENTED_YET("Allocation type doesn't match platform! This implies a weird bug!\n", "Windows (VirtualFreeEx)" );
        return NULL; // 
      #endif
    }
#else /*!NUMA*/
    NOT_IMPLEMENTED_YET("Moving NUMA NODE Unsupported without NUMA!\n", "Only supported on Windows and Linux with NUMA enabled");
    //fprintf(stderr, "");
    return 0; //    
#endif /*NUMA*/
}


int common_numa_check_available_node_count(){
#if 1//def NUMA
    #if IS_WINDOWS(ENVTYPE)
    ULONG highestNumaNode;
    if (!GetNumaHighestNodeNumber(&highestNumaNode)) {
        fprintf(stderr, "Could not get highest NUMA node number: %d\n", GetLastError());
        return -1;
    }

    #elif IS_GCC_POSIX(ENVTYPE)
    if (numa_available() == -1) {
        fprintf(stderr, "NUMA is not available\n");
        return -1;
    }

    int numaNodeCount = numa_max_node() + 1;
    if (numaNodeCount > 64) {
        fprintf(stderr, "Too many NUMA nodes (%d). Go home.\n", numaNodeCount);
        return -numaNodeCount;
    }
    return numaNodeCount;

    #endif
#else /*!NUMA*/
    NOT_IMPLEMENTED_YET("This version is compiled without NUMA support!\n", "Only supported on Windows and Linux with NUMA enabled");
    //fprintf(stderr, "");
    return 0; //    
#endif /*NUMA*/
}

int common_numa_set_cpu_affinity_to_node(int target_cpu_node){
#ifdef NUMA
    #if IS_WINDOWS(ENVTYPE)
    ULONGLONG mask;
    DWORD index;
    
    if (GetNumaNodeProcessorMask(coreNode, &mask) == 0) {// 0 means fail?
        fprintf(stderr, "GetNumaNodeProcessorMask Failed: %d\n", GetLastError());
        return -1;
    }
    BitScanReverse64(&index, mask);
    mask = 0;
    mask |= 1ULL << (ULONGLONG)index;
    if (SetProcessAffinityMask(GetCurrentProcess(), mask) == 0) {// 0 means fail?
        fprintf(stderr, "SetProcessAffinityMask Failed: %d\n", GetLastError());
        return -1;
    }
    return 0;

    #elif IS_GCC_POSIX(ENVTYPE)
    // Why like this and not use numa_run_on_node()? I based on existing implem. because I assume it did things for a reason.
    struct bitmask *nodeBitmask = numa_allocate_cpumask();
    if (nodeBitmask == NULL){ // Although it's not clear I should be doing this (documentation unclear), I assume null is bad
        fprintf(stderr, "Allocating CPU mask for node %d failed!\n", target_cpu_node);
        return -1;
    }
    
    if (numa_node_to_cpus(target_cpu_node, nodeBitmask) < 0){
        fprintf(stderr, "numa_node_to_cpus Failed, errno %d = %s\n", errno, strerror(errno));
        numa_free_cpumask(nodeBitmask);
        return -1;
    }
    int nodeCpuCount = numa_bitmask_weight(nodeBitmask);
    if (nodeCpuCount == 0) {
        fprintf(stderr, "Node %d has no cores\n", cpuNode);
        return -2;
    }

    fprintf(stderr, "Node %d has %d cores\n", cpuNode, nodeCpuCount);
    cpu_set_t cpuset;
    memcpy(cpuset.__bits, nodeBitmask->maskp, nodeBitmask->size / 8);
    // for (int i = 0; i < get_nprocs(); i++) 
    //  if (numa_bitmask_isbitset(nodeBitmask, i)) CPU_SET(i, &cpuset); 

    if (sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpuset) == -1){
        fprintf(stderr, "Failed to set affinity, errno %d = %s\n", errno, strerror(errno));
        numa_free_cpumask(nodeBitmask);
        return -1;
    }
    numa_free_cpumask(nodeBitmask);
    return 0;
    #endif
#else /*!NUMA*/
    NOT_IMPLEMENTED_YET("This version is compiled without NUMA support!\n", "Only supported on Windows and Linux with NUMA enabled");
    //fprintf(stderr, "");
    return 0; //    
#endif /*NUMA*/
}


#endif /*COMMON_MEMORY_H*/
