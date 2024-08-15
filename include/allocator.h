#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "test_types.h"

#define CL_SIZE 64

int getsizeof(TestConfigStreamType);
size_t getstreamelems(RuntimeStreamConfig *sdata);

int init_function(void* ptr, int state, TestConfigStreamType type, int dims, int64_t* dimsizes, void* init_val, ...);

int allocate_arrays(RuntimeStreamConfig *sdata);
void release_arrays(RuntimeStreamConfig *sdata);

int initialize_arrays(RuntimeStreamConfig *sdata);
int allocate_streams(RuntimeConfig* runcfg);

int print_arrays(RuntimeStreamConfig *sdata);

#endif
