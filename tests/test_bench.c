#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "bench.h"
#include "error.h"
#include "test_types.h"
#include "timer.h"


int global_verbosity = DEBUGLEV_ONLY_ERROR;


static RuntimeThreadConfig* curr_config;

void myfunc(void)
{
    RuntimeThreadConfig* cfg = (RuntimeThreadConfig*)curr_config;
    size_t size = cfg->sdata[0].dimsizes[0];
    void* ptr = cfg->sdata[0].ptr;
    int* iptr = (int*)ptr;
    for (int i = 0; i < size/sizeof(int); i++)
    {
        iptr[i] = 43;
    }
}

int main(int argc, char* argv) {

    printf("Preparing benchmark input\n");
    RuntimeThreadConfig *tconfig = malloc(sizeof(RuntimeThreadConfig));
    if (!tconfig)
    {
        return -ENOMEM;
    }
    curr_config = tconfig;
    thread_data_t data = malloc(sizeof(_thread_data));
    if (!data)
    {
        free(tconfig);
        return -ENOMEM;
    }
    memset(data, 0, sizeof(_thread_data));
    tconfig->command = malloc(sizeof(RuntimeThreadCommand));
    if (!tconfig->command)
    {
        free(data);
        free(tconfig);
        return -ENOMEM;
    }
    memset(tconfig->command, 0, sizeof(RuntimeThreadCommand));
    tconfig->sdata = malloc(2*sizeof(RuntimeStreamConfig));
    if (!tconfig->sdata)
    {
        free(data);
        free(tconfig->command);
        free(tconfig);
        return -ENOMEM;
    }
    memset(tconfig->sdata, 0, sizeof(RuntimeStreamConfig));
    tconfig->sdata[0].ptr = malloc(10000*sizeof(int));
    if (!tconfig->sdata[0].ptr)
    {
        free(tconfig->sdata);
        free(tconfig->command);
        free(tconfig->data);
        free(tconfig);
        return -ENOMEM;
    }
    tconfig->sdata[0].dimsizes[0] = 10000*sizeof(int);
    tconfig->sdata[0].dims = 1;
    tconfig->sdata[0].id = 0;
    tconfig->sdata[0].offsets[0] = 0;
    tconfig->sdata[0].type = TEST_STREAM_TYPE_INT;
    tconfig->num_streams = 1;
    tconfig->command->cmdfunc.run = (BenchFuncPrototype)myfunc;
    tconfig->command->cmd = LIKWID_THREAD_COMMAND_RUN;

    data->hwthread = 0;
    data->iters = 1000;
    data->min_runtime = 10;
    tconfig->data = data;

    tconfig->barrier = NULL;
    printf("running benchmark\n");
    run_benchmark(tconfig);
    printf("benchmark finished in %f seconds\n", tconfig->runtime);
    free(tconfig->sdata[0].ptr);
    free(tconfig->sdata);
    free(tconfig->command);
    free(tconfig->data);
    free(tconfig);

    return 0;
}
