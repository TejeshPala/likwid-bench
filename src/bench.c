#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "bench.h"
#include "error.h"
#include "timer.h"
#include "test_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIKWID_PERFMON
#include "likwid-marker.h"
#endif

#ifdef __cplusplus
extern "C" }
#endif

#define DECLARE_TIMER TimerDataLB timedata

// todo markers
// todo timer library with rdtsc? or perf?
#ifdef LIKWID_PERFMON
#define EXECUTE(func) \
    LIKWID_MARKER_REGISTER("LIKWID-BENCH"); \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \
    if (lb_timer_init(TIMER_RDTSC, &timedata) != 0) fprintf(stderr, "Timer initialization failed!\n"); \
    LIKWID_MARKER_START("LIKWID-BENCH"); \
    lb_timer_start(&timedata); \
    for (int i = 0; i < myData->iters; i++) \
    {   \
        func; \
    } \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \
    lb_timer_stop(&timedata); \
    LIKWID_MARKER_STOP("LIKWID-BENCH"); \
    lb_timer_as_ns(&timedata, &myData->min_runtime); \
    lb_timer_as_cycles(&timedata, &myData->cycles); \
    myData->freq = timedata.ci.freq; \
    lb_timer_close(&timedata); \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier);
#else
#define EXECUTE(func) \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \
    if (lb_timer_init(TIMER_RDTSC, &timedata) != 0) fprintf(stderr, "Timer initialization failed!\n"); \
    lb_timer_start(&timedata); \
    for (int i = 0; i < myData->iters; i++) \
    {   \
        func; \
    } \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier); \
    lb_timer_stop(&timedata); \
    lb_timer_as_ns(&timedata, &myData->min_runtime); \
    lb_timer_as_cycles(&timedata, &myData->cycles); \
    myData->freq = timedata.ci.freq; \
    lb_timer_close(&timedata); \
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier);
#endif

int run_benchmark(RuntimeThreadConfig* data)
{
    cpu_set_t cpuset;
    cpu_set_t runset;
    thread_data_t myData = data->data;
    BenchFuncPrototype func = data->command->cmdfunc.run;
    DECLARE_TIMER;

    // not sure whether this is required or the threads are already pinned
    // but helpful for testing
    sched_getaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (CPU_COUNT(&cpuset) > 1)
    {
        // Pinning gets reset later to the original cpuset
        CPU_ZERO(&runset);
        CPU_SET(myData->hwthread, &runset);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Pinning task to hwthread %d", myData->hwthread);
        sched_setaffinity(0, sizeof(cpu_set_t), &runset);
    }

    if (!data->barrier) DEBUG_PRINT(DEBUGLEV_DEVELOP, "Run in serial mode");
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier);

    /* Up to 10 streams the following registers are used for Array ptr:
     * Size rdi
     * in Registers: rsi  rdx  rcx  r8  r9
     * passed on stack, then: r10  r11  r12  r13  r14  r15
     * If more than 10 streams are used first 5 streams are in register, above 5 a macro must be used to
     * load them from stack
     * */

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "hwthread %3d starts benchmark execution: %s", myData->hwthread, ctime(&ts.tv_sec));

    EXECUTE(func());
    // not sure whether we need to give the sizes here. Since we compile the code, we could add the sizes there directly
    // as constants
    /*
    switch (data->num_streams)
    {
        case 1:
            EXECUTE(func(data->sdata[0].dimsizes[0], data->sdata[0].ptr));
            break;
        case 2:
            EXECUTE(func(data->sdata[0].dimsizes[0], data->sdata[0].ptr, data->sdata[1].ptr));
            break;
        case 3:
            EXECUTE(func(data->sdata[0].dimsizes[0], data->sdata[0].ptr, data->sdata[1].ptr, data->sdata[2].ptr));
            break;
        case 4:
            EXECUTE(func(data->sdata[0].dimsizes[0], data->sdata[0].ptr, data->sdata[1].ptr, data->sdata[2].ptr, data->sdata[3].ptr));
            break;
            */
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
    /*
        default:
            break;
    }
    */

    data->runtime = (double)myData->min_runtime / NANOS_PER_SEC;
    data->cycles = myData->cycles;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "hwthread %3d execution took %.15f seconds", myData->hwthread, data->runtime);
    if (data->barrier) pthread_barrier_wait(&data->barrier->barrier);

    if (CPU_COUNT(&runset) > 0)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Reset task affinity to %d hwthreads", CPU_COUNT(&cpuset));
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    }
    return 0;
}
