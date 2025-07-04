#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <unistd.h>


#include "test_types.h"

static inline size_t get_cl_size()
{
    static size_t c_cl_size = 0;
    if (c_cl_size == 0)
    {
        size_t cl_size = (size_t)sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
        // fprintf(stdout, "CL_SIZE from SYSCONF: %zu Bytes\n", cl_size);
        c_cl_size = ((cl_size > 0 && !(cl_size & 7)) ? cl_size : 64);
        // fprintf(stdout, "Final CL_SIZE from SYSCONF: %zu Bytes\n", c_cl_size);
    }
    return c_cl_size;
}

#define CL_SIZE get_cl_size()

size_t getsizeof(TestConfigStreamType);
size_t getstreamelems(RuntimeStreamConfig *sdata);
size_t getstreambytes(RuntimeStreamConfig *sdata);

int init_function(void* ptr, int state, TestConfigStreamType type, int dims, size_t* dimsizes, void* init_val, ...);

int allocate_arrays(RuntimeStreamConfig *sdata);
void release_arrays(RuntimeStreamConfig *sdata);

int initialize_arrays(RuntimeStreamConfig *sdata);
int allocate_streams(RuntimeConfig* runcfg);

int print_arrays(RuntimeStreamConfig *sdata);

#endif
