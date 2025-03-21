#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <unistd.h>


#include "test_types.h"

static inline size_t get_cl_size()
{
    size_t cl_size = (size_t)sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    // fprintf(stdout, "CL_SIZE from SYSCONF: %zu Bytes\n", cl_size);
    return ((cl_size > 0 && cl_size <= 64) ? cl_size : 64);
}

#define CL_SIZE get_cl_size()

int getsizeof(TestConfigStreamType);
size_t getstreamelems(RuntimeStreamConfig *sdata);

int init_function(void* ptr, int state, TestConfigStreamType type, int dims, int64_t* dimsizes, void* init_val, ...);

int allocate_arrays(RuntimeStreamConfig *sdata);
void release_arrays(RuntimeStreamConfig *sdata);

int initialize_arrays(RuntimeStreamConfig *sdata);
int allocate_streams(RuntimeConfig* runcfg);

int print_arrays(RuntimeStreamConfig *sdata);

#endif
