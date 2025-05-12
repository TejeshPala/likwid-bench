/*
 * =======================================================================================
 *      Filename:  stack.h
 *
 *      Description:  The main header defining the test data types
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:  Thomas Gruber (tg), thomas.roehl@gmail.com
 *      Project:  likwid-bench
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "bstrlib.h"
#include "map.h"
#include "bitmap.h"

typedef struct {
    bstring                 name;
    bstring                 description;
    struct bstrList*        options;
    bstring                 defvalue;
} TestConfigParameter;

typedef enum {
    TEST_STREAM_TYPE_SINGLE = 0,
    TEST_STREAM_TYPE_DOUBLE = 1,
    TEST_STREAM_TYPE_INT    = 2,
    TEST_STREAM_TYPE_HALF   = 3,
    TEST_STREAM_TYPE_INT64  = 4,
    MAX_DATA_TYPE
} TestConfigStreamType;
#define MIN_DATA_TYPE 0

typedef enum {
    TEST_STREAM_ALLOC_TYPE_1DIM = 0,
    TEST_STREAM_ALLOC_PERTHREAD,
    MAX_STREAM_TYPE
} TestConfigStreamFlag;
#define MIN_STREAM_TYPE 0

typedef union {
    float       fval;
    double      dval;
    int         ival;
#ifdef WITH_HALF_PRECISION
    _Float16    f16val;
#endif
    int64_t     i64val;
} TestConfigStreamData;

typedef struct {
    bstring                 name;
    int                     num_dims;
    TestConfigStreamType    type;
    bstring                 btype;
    struct bstrList*        dims;
    TestConfigStreamData    data;
    struct bstrList*        sizes;
    struct bstrList*        offsets;
    bool                    initialization;
} TestConfigStream;

typedef struct {
    bstring                 name;
    bstring                 value;
} TestConfigVariable;

typedef struct {
    bstring                 name;
    bstring                 description;
    bstring                 language;
    bstring                 code;
    int                     num_params;
    TestConfigParameter *   params;
    int                     num_streams;
    TestConfigStream *      streams;
    int                     num_constants;
    TestConfigVariable *    constants;
    int                     num_vars;
    TestConfigVariable *    vars;
    int                     num_metrics;
    TestConfigVariable *    metrics;
    struct bstrList*        flags;
    bool                    requirewg;
} TestConfig;
typedef TestConfig* TestConfig_t;

typedef enum {
    STREAM_DIM_1D = 1,
    STREAM_DIM_2D = 2,
    STREAM_DIM_3D = 3
} StreamDimension;

typedef struct {
    bstring name;
    void* ptr;
    void* base_ptr;
    uint64_t dimsizes[3];
    off_t offsets[3];
    int dims;
    Bitmap flags;
    int id;
    int (*init)(void* ptr, int state, TestConfigStreamType type, int dims, uint64_t* dimsizes, void* init_val, ...);
    TestConfigStreamType type;
    TestConfigStreamData data;
    void* init_val;
    bool initialization;
} RuntimeStreamConfig;

typedef struct {
    int num_aggregations;
    struct tagbstring baggregations[4];
} Aggregations;

static const Aggregations aggregations = {
    .num_aggregations = 4,
    .baggregations = {
        bsStatic("min"),
        bsStatic("max"),
        bsStatic("sum"),
        bsStatic("median"),
    }
};

typedef struct {
    Map_t values;
    Map_t variables;
} RuntimeWorkgroupResult;

typedef struct {
    bstring name;
    bstring value;
    struct bstrList* values;
} RuntimeParameterConfig;

typedef struct {
    bstring objfile;
    bstring functionname;
    bstring flags;
    bstring compiler;
    void* dlhandle;
    void* function;
} RuntimeTestConfig;

typedef enum {
    LIKWID_THREAD_COMMAND_EXIT = 0,
    LIKWID_THREAD_COMMAND_NOOP,
    LIKWID_THREAD_COMMAND_INITIALIZE,
    LIKWID_THREAD_COMMAND_RUN,
    LIKWID_THREAD_COMMAND_VERIFY,
} LikwidThreadCommand;

typedef struct Queue {
    LikwidThreadCommand cmd;
    struct Queue* next;
} Queue;

typedef struct {
    LikwidThreadCommand cmd;
    Queue* head;
    Queue* tail;
    union {
        int (*exit)();
        void (*run)();
    } cmdfunc;
    int done;
    pthread_attr_t attr;
    pthread_mutex_t mutex;
    pthread_mutexattr_t m_attr;
    pthread_cond_t cond;
    pthread_condattr_t c_attr;
    void* init_val;
} RuntimeThreadCommand;

typedef struct {
    pthread_barrier_t barrier;
    pthread_barrierattr_t b_attr;
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
    uint64_t freq;
    uint64_t min_runtime;
    //const TestConfig_t test;
    int hwthread;
    int flags;
} _thread_data;
typedef _thread_data* thread_data_t;

typedef struct {
    uint64_t    tsizes[3];
    off_t       toffsets[3];
    void*       tstream_ptr;
} RuntimeThreadStreamConfig;

typedef struct {
    pthread_t thread;
    int local_id;
    int global_id;
    double runtime;
    uint64_t cycles;
    thread_barrier_t* barrier;
    thread_data_t data;
    RuntimeThreadCommand* command;
    int num_threads;
    RuntimeTestConfig* testconfig;
    struct bstrList* codelines;
    int num_streams;
    RuntimeStreamConfig* sdata;
    RuntimeThreadStreamConfig* tstreams;
} RuntimeThreadConfig;

typedef struct {
    bstring str;
    int num_threads;
    int* hwthreads;
    RuntimeThreadConfig* threads;
    thread_barrier_t barrier;
    RuntimeWorkgroupResult* results;
    RuntimeWorkgroupResult* group_results;
    int num_streams;
    RuntimeStreamConfig* streams;
} RuntimeWorkgroupConfig;

typedef struct {
    int help;
    int verbosity;
    uint64_t iterations;
    double runtime;
    int csv;
    int json;
    int detailed;
    bstring output;
    int num_wgroups;
    RuntimeWorkgroupConfig* wgroups;
    int num_params;
    RuntimeParameterConfig* params;
    bstring testname;
    bstring pttfile;
    bstring compiler;
    bstring kernelfolder;
    bstring tmpfolder;
    bstring arraysize;
    TestConfig_t tcfg;
    RuntimeWorkgroupResult* global_results;
} RuntimeConfig;


#endif /* TEST_TYPES_H */
