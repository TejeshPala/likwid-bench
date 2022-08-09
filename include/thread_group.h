#ifndef THREAD_GROUP_H
#define THREAD_GROUP_H

/*#include <stdlib.h>*/
/*#include <stdio.h>*/
/*#include <string.h>*/
/*#include <unistd.h>*/
#include <stdint.h>

#include <pthread.h>

/*#include <map.h>*/

typedef struct {
    pthread_barrier_t barrier;
    pthread_barrier_attr_t b_attr;
} thread_barrier_t;

typedef enum {
    THREAD_DATA_THREADINIT = 0,
    THREAD_DATA_MAX,
} _thread_data_flags;
#define THREAD_DATA_MIN THREAD_DATA_FLAG_THREADINIT

#define THREAD_DATA_THREADINIT_FLAG (1<<THREAD_DATA_THREADINIT)

typedef struct {
    uint64_t iters;
    uint64_t cycles;
    uint64_t min_runtime;
    //const TestConfig_t test;
    int hwthread;
    int flags;
} _thread_data;
typedef _thread_data* thread_data_t;

typedef struct {
    pthread thread;
    int local_id;
    int global_id;
    double runtime;
    uint64_t cycles;
    thread_barrier_t* barrier;
    thread_data_t data;
} thread_group_thread_t;

typedef struct {
    int id;
    int workgroup;
    int num_threads;
    thread_group_thread_t *threads;
    thread_barrier_t* barrier;
} thread_group_t;

#endif
