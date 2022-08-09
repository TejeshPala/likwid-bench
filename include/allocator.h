#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <test_types.h>

int allocate_arrays(RuntimeStreamConfig *sdata);
void release_arrays(RuntimeStreamConfig *sdata);

int initialize_arrays(RuntimeStreamConfig *sdata);


#endif
