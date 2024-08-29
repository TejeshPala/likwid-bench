#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>
#include <test_types.h>
#include <bench.h>
#include <sched.h>


// todo markers
// todo timer library with rdtsc? or perf?
#define EXECUTE(func) \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \
    clock_gettime(CLOCK_MONOTONIC, &start); \
    for (int i = 0; i < myData->iters; i++) \
    {   \
        func; \
    } \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \
    clock_gettime(CLOCK_MONOTONIC, &stop); \
    data->runtime = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9)); \
    printf("Runtime %f\n", data->runtime); \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \

void* run_benchmark(void* arg)
{
    cpu_set_t cpuset;
    cpu_set_t runset;
    RuntimeThreadConfig *data = (RuntimeThreadConfig*)arg;
    thread_data_t myData = data->data;
    BenchFuncPrototype func = data->command->cmdfunc.run;
    struct timespec start = {0, 0}, stop = {0, 0};

    // not sure whether this is required or the threads are already pinned
    // but helpful for testing
    sched_getaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (CPU_COUNT(&cpuset) > 1)
    {
        // Pinning gets reset later to the original cpuset
        CPU_ZERO(&runset);
        CPU_SET(myData->hwthread, &runset);
        printf("Pinning task to hwthread %d\n", myData->hwthread);
        sched_setaffinity(0, sizeof(cpu_set_t), &runset);
    }

    if (!data->barrier) printf("Run in serial mode\n");
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier);

    /* Up to 10 streams the following registers are used for Array ptr:
     * Size rdi
     * in Registers: rsi  rdx  rcx  r8  r9
     * passed on stack, then: r10  r11  r12  r13  r14  r15
     * If more than 10 streams are used first 5 streams are in register, above 5 a macro must be used to
     * load them from stack
     * */

    // not sure whether we need to give the sizes here. Since we compile the code, we could add the sizes there directly
    // as constants
    switch ( data->command->num_streams ) {
        case 1:
            EXECUTE(func(data->command->tstreams[0].dimsizes[0], data->command->tstreams[0].ptr));
            break;
        case 2:
            EXECUTE(func(data->command->tstreams[0].dimsizes[0], data->command->tstreams[0].ptr, data->command->tstreams[1].ptr));
            break;
        case 3:
            EXECUTE(func(data->command->tstreams[0].dimsizes[0], data->command->tstreams[0].ptr, data->command->tstreams[1].ptr, data->command->tstreams[2].ptr));
            break;
/*        case STREAM_4:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3]));*/
/*            break;*/
/*        case STREAM_5:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4]));*/
/*            break;*/
/*        case STREAM_6:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5]));*/
/*            break;*/
/*        case STREAM_7:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6]));*/
/*            break;*/
/*        case STREAM_8:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7]));*/
/*            break;*/
/*        case STREAM_9:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8]));*/
/*            break;*/
/*        case STREAM_10:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9]));*/
/*            break;*/
/*        case STREAM_11:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10]));*/
/*            break;*/
/*        case STREAM_12:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11]));*/
/*            break;*/
/*        case STREAM_13:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12]));*/
/*            break;*/
/*        case STREAM_14:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13]));*/
/*            break;*/
/*        case STREAM_15:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14]));*/
/*            break;*/
/*        case STREAM_16:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15]));*/
/*            break;*/
/*        case STREAM_17:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16]));*/
/*            break;*/
/*        case STREAM_18:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17]));*/
/*            break;*/
/*        case STREAM_19:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18]));*/
/*            break;*/
/*        case STREAM_20:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19]));*/
/*            break;*/
/*        case STREAM_21:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20]));*/
/*            break;*/
/*        case STREAM_22:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21]));*/
/*            break;*/
/*        case STREAM_23:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22]));*/
/*            break;*/
/*        case STREAM_24:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23]));*/
/*            break;*/
/*        case STREAM_25:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24]));*/
/*            break;*/
/*        case STREAM_26:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25]));*/
/*            break;*/
/*        case STREAM_27:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26]));*/
/*            break;*/
/*        case STREAM_28:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27]));*/
/*            break;*/
/*        case STREAM_29:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28]));*/
/*            break;*/
/*        case STREAM_30:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29]));*/
/*            break;*/
/*        case STREAM_31:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30]));*/
/*            break;*/
/*        case STREAM_32:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31]));*/
/*            break;*/
/*        case STREAM_33:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31],*/
/*                        myData->streams[32]));*/
/*            break;*/
/*        case STREAM_34:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31],*/
/*                        myData->streams[32],myData->streams[33]));*/
/*            break;*/
/*        case STREAM_35:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31],*/
/*                        myData->streams[32],myData->streams[33],myData->streams[34]));*/
/*            break;*/
/*        case STREAM_36:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31],*/
/*                        myData->streams[32],myData->streams[33],myData->streams[34],myData->streams[35]));*/
/*            break;*/
/*        case STREAM_37:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31],*/
/*                        myData->streams[32],myData->streams[33],myData->streams[34],myData->streams[35],*/
/*                        myData->streams[36]));*/
/*            break;*/
/*        case STREAM_38:*/
/*            EXECUTE(func(size,myData->streams[0],myData->streams[1],myData->streams[2],myData->streams[3],*/
/*                        myData->streams[4],myData->streams[5],myData->streams[6],myData->streams[7],*/
/*                        myData->streams[8],myData->streams[9],myData->streams[10],myData->streams[11],*/
/*                        myData->streams[12],myData->streams[13],myData->streams[14],myData->streams[15],*/
/*                        myData->streams[16],myData->streams[17],myData->streams[18],myData->streams[19],*/
/*                        myData->streams[20],myData->streams[21],myData->streams[22],myData->streams[23],*/
/*                        myData->streams[24],myData->streams[25],myData->streams[26],myData->streams[27],*/
/*                        myData->streams[28],myData->streams[29],myData->streams[30],myData->streams[31],*/
/*                        myData->streams[32],myData->streams[33],myData->streams[34],myData->streams[35],*/
/*                        myData->streams[36],myData->streams[37]));*/
/*            break;*/
        default:
            break;
    }

    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier);
    if (data->barrier) pthread_exit(NULL);

    if (CPU_COUNT(&runset) > 0)
    {
        printf("Reset task affinity to %d hwthreads\n", CPU_COUNT(&cpuset));
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    }

    return NULL;
}
