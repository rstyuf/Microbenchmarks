#define ALLOCATOR_MODULE (true)

#include "common_memory.h"

#define MAX_ALLOCATIONS_PER_RUN (128)

struct allocation_record_t allocations[MAX_ALLOCATIONS_PER_RUN] = {0};

