#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include <bstrlib.h>

typedef struct {
    bstring                 name;
    bstring                 description;
    struct bstrList*        options;
} TestConfigParameter;

typedef enum {
    TEST_STREAM_TYPE_SINGLE,
    TEST_STREAM_TYPE_DOUBLE,
    TEST_STREAM_TYPE_INT,
} TestConfigStreamType;

typedef enum {
    TEST_STREAM_ALLOC_TYPE_1DIM = 0,
    TEST_STREAM_ALLOC_PERTHREAD,
} TestConfigStreamFlag;

typedef struct {
    bstring                 name;
    int                     num_dims;
    TestConfigStreamType    type;
    bstring                 btype;
    struct bstrList*        dims;
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
} TestConfig;
typedef TestConfig* TestConfig_t;

typedef struct {
    void* ptr;
    int dimsizes[3];
    int offsets[3];
    int dims;
    int flags;
    int id;
    int (*init)(void* ptr, int state, int dims, ...);
    TestConfigStreamType type;
} RuntimeStreamConfig;

typedef struct {
    int num_threads;
    int* cpulist;
    pthread_t *threads;
} RuntimeWorkgroupConfig;

typedef struct {
    int iterations;
    double runtime;
    int num_wgroups;
    RuntimeWorkgroupConfig* wgroups;
} RuntimeConfig;


#endif
