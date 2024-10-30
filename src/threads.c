#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <pthread.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include "test_types.h"
#include "thread_group.h"
#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "allocator.h"
#include "calculator.h"
#include "results.h"

#if defined(_GNU_SOURCE) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 4)) && HAS_SCHEDAFFINITY
    #define USE_PTHREAD_AFFINITY 1
#else
    #define USE_PTHREAD_AFFINITY 0
#endif

#ifdef __linux__
#define gettid() syscall(SYS_gettid)
#else
#define gettid() pthread_self()
#endif

int destroy_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying threads for workgroup %3d, w);
        pthread_barrier_destroy(&wg->barrier.barrier);
        if (wg->threads)
        {
            for (int i = 0; i < wg->num_threads; i++)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread %3d for workgroup %3d, i, w);
                if (wg->threads[i].data)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying data for thread %3d, wg->threads[i].local_id);
                    free(wg->threads[i].data);
                    wg->threads[i].data = NULL;
                }
                if (wg->threads[i].command)
                {
                    if (wg->threads[i].command->init_val)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying init_val for thread %3d, wg->threads[i].local_id);
                        free(wg->threads[i].command->init_val);
                        wg->threads[i].command->init_val = NULL;
                    }
                    pthread_mutex_destroy(&wg->threads[i].command->mutex);
                    pthread_cond_destroy(&wg->threads[i].command->cond);
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying commands for thread %3d, wg->threads[i].local_id);
                    free(wg->threads[i].command);
                    wg->threads[i].command = NULL;
                }
            }
            free(wg->threads);
            wg->threads = NULL;
        }
    }
    return 0;
}

int _set_t_aff(pthread_t thread, int cpuid)
{
    int err = 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuid, &cpuset);
    if (cpuid < 0 || cpuid >= CPU_SETSIZE)
    {
        ERROR_PRINT(Invalid cpu id: %d, cpuid);
        return -EINVAL;
    }

    if (USE_PTHREAD_AFFINITY)
    {
        // printf("GLIBC MAJOR: %d, MINOR: %d\n", __GLIBC__, __GLIBC_MINOR__);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Using pthread_setaffinity_np);
        err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
        if (err != 0)
        {
            ERROR_PRINT(Error setting pthread affinity %s, strerror(err));
            return err;
        }
    }
    else
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Using sched_setaffinity);
        pid_t pid = gettid();
        err = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);
        if (err != 0)
        {
            ERROR_PRINT(Error setting PID thread affinity %s, strerror(err));
            return err;
        }
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, The CPU´s in the set are: %d, CPU_COUNT(&cpuset));
    return 0;
}

void _print_aff(pthread_t thread)
{
    int err = 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    if (USE_PTHREAD_AFFINITY)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Using pthread_getaffinity_np);
        err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
        if (err != 0)
        {
            ERROR_PRINT(Error getting pthread affinity %s, strerror(err));
            return;
        }
    }
    else
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Using sched_getaffinity);
        pid_t pid = gettid();
        err = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
        if (err != 0)
        {
            ERROR_PRINT(Error getting PID thread affinity %s, strerror(err));
            return;
        }
    }

    printf("Threadid %16lu -> hwthread/affinity: ", thread);

    for (size_t t = 0; t < CPU_SETSIZE; t++)
    {
        if (CPU_ISSET(t, &cpuset))
        {
            printf("%3ld ", t);
        }
    }
    printf("\n");
    DEBUG_PRINT(DEBUGLEV_DEVELOP, The CPU´s in the set are: %d, CPU_COUNT(&cpuset));
}

double get_time_s()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int initialize_local(RuntimeThreadConfig* thread, int thread_id)
{
    int err = 0;
    for (int s = 0; s < thread->command->num_streams; s++)
    {
        RuntimeStreamConfig* sdata = &thread->command->tstreams[s];
        // DEBUG_PRINT(DEBUGLEV_DEVELOP, dims: %d, sdata->dims);
        size_t elems = getstreamelems(sdata);
        size_t offset;
        size_t size;
        if (thread->sizes > 0 && thread->offsets >= 0)
        {
            offset = thread->offsets;
            size = thread->sizes;
        }
        else
        {
            elems = getstreamelems(sdata);
            offset = 0;
            size = elems;
        }
        if (thread->num_threads > 1 && (thread->sizes == 0 && thread->offsets == 0))
        {
            size_t chunk = elems / thread->num_threads;
            size_t rem_chunk = elems % thread->num_threads;
            // printf("num elems: %ld, num threads: %ld, chunk: %ld\n", getstreamelems(thread->command->tstreams), thread->num_threads, chunk);
            int local_id = thread_id % thread->num_threads;
            offset = local_id * chunk + (local_id < rem_chunk ? local_id : rem_chunk);
            size = chunk + (local_id < rem_chunk ? 1 : 0);
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d initializing stream %d with total elements: %ld offset: %ld, thread_id, s, elems, offset);
        sdata->init_val = thread->command->init_val;
        RuntimeStreamConfig tmp = *sdata;
        tmp.dims = sdata->dims;
        switch ((StreamDimension)sdata->dims)
        {
            case STREAM_DIM_1D:
                {
                    tmp.ptr = (char*)sdata->ptr + (offset * getsizeof(sdata->type));
                    tmp.dimsizes[0] = size * getsizeof(sdata->type);
                    break;
                }
            case STREAM_DIM_2D:
                {
                    size_t rows = sdata->dimsizes[0] / getsizeof(sdata->type);
                    size_t cols = sdata->dimsizes[1] / getsizeof(sdata->type);
                    size_t start_rows = offset / cols;
                    size_t end_rows = (offset + size - 1) / cols;
                    tmp.ptr = (char*)sdata->ptr + (start_rows * cols);
                    tmp.dimsizes[0] = (end_rows - start_rows + 1) * getsizeof(sdata->type);
                    tmp.dimsizes[1] = cols * getsizeof(sdata->type);
                    break;
                }
            case STREAM_DIM_3D:
                {
                    size_t dim1 = sdata->dimsizes[0] / getsizeof(sdata->type);
                    size_t dim2 = sdata->dimsizes[1] / getsizeof(sdata->type);
                    size_t dim3 = sdata->dimsizes[2] / getsizeof(sdata->type);
                    size_t slice = dim2 * dim3;
                    size_t start = offset / slice;
                    size_t end = (offset + size - 1) / slice;
                    tmp.ptr = (char*)sdata->ptr + (start * slice);
                    tmp.dimsizes[0] = (end - start + 1) * getsizeof(sdata->type);
                    tmp.dimsizes[1] = dim2 * getsizeof(sdata->type);
                    tmp.dimsizes[2] = dim3 * getsizeof(sdata->type);
                    break;
                }
        }
        tmp.type = sdata->type;
        tmp.init = init_function;
        tmp.init_val = sdata->init_val;

        err = initialize_arrays(&tmp);
        if (err != 0)
        {
            ERROR_PRINT(Initialization failed for stream %d, s);
        }
        // print_arrays(sdata);
    }

    return err;
}

int initialize_global(RuntimeThreadConfig* thread)
{
    int err = 0;
    for (int i = 0; i < thread->command->num_streams; i++)
    {
        RuntimeStreamConfig* sdata = &thread->command->tstreams[i];
        sdata->init_val = thread->command->init_val;
        RuntimeStreamConfig tmp = *sdata;
        tmp.ptr = (char*)sdata->ptr;
        tmp.dims = sdata->dims;
        switch ((StreamDimension)sdata->dims)
        {
            case STREAM_DIM_1D:
                {
                    tmp.dimsizes[0] = sdata->dimsizes[0];
                    break;
                }
            case STREAM_DIM_2D:
                {
                    tmp.dimsizes[0] = sdata->dimsizes[0];
                    tmp.dimsizes[1] = sdata->dimsizes[1];
                    break;
                }
            case STREAM_DIM_3D:
                {
                    tmp.dimsizes[0] = sdata->dimsizes[0];
                    tmp.dimsizes[1] = sdata->dimsizes[1];
                    tmp.dimsizes[2] = sdata->dimsizes[2];
                    break;
                }
        }
        tmp.type = sdata->type;
        tmp.init = init_function;
        tmp.init_val = thread->command->init_val;

        err = initialize_arrays(&tmp);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize for stream %d, i);
            return err;
        }
        // print_arrays(sdata);
    }

    return err;
}

void* _func_t(void* arg)
{
    RuntimeThreadConfig* thread = (RuntimeThreadConfig*)arg;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d is running, thread->local_id, thread->global_id);
    while (1)
    {
        pthread_mutex_lock(&thread->command->mutex);
        while(thread->command->done)
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += TIMEOUT_SECONDS;
            int err = pthread_cond_timedwait(&thread->command->cond, &thread->command->mutex, &ts);
            if (err != 0)
            {
                if (thread->command->cmd != LIKWID_THREAD_COMMAND_NOOP && err == ETIMEDOUT)
                {
                    ERROR_PRINT(Thread %d timedout waiting for command, thread->local_id);
                }
                pthread_mutex_unlock(&thread->command->mutex);
                goto exit_thread;
            }
            // pthread_cond_wait(&thread->command->cond, &thread->command->mutex);
        }
        LikwidThreadCommand c_cmd = thread->command->cmd;
        pthread_mutex_unlock(&thread->command->mutex);

        // DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d received cmd %d, thread->local_id, thread->global_id, c_cmd);

        switch(c_cmd)
        {
            case LIKWID_THREAD_COMMAND_INITIALIZE:
                if (thread->global_id == 0 && !thread->command->initialization)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Global Initialization on thread %d with global thread %d, thread->local_id, thread->global_id);
                    int err = initialize_global(thread);
                    if (err != 0)
                    {
                        ERROR_PRINT(Global Initialization failed for thread %d with global thread %d, thread->local_id, thread->global_id);
                    }
                }
                else if (thread->command->initialization)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Local Initialization on thread %d with global thread %d, thread->local_id, thread->global_id);
                    int err = initialize_local(thread, thread->local_id);
                    if (err != 0)
                    {
                        ERROR_PRINT(Local Initialization failed for thread %d with global thread %d, thread->local_id, thread->global_id);
                    }
                }
                break;
            case LIKWID_THREAD_COMMAND_NOOP:
                DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d is set with NOOP command, thread->local_id, thread->global_id);
                break;
            case LIKWID_THREAD_COMMAND_EXIT:
                DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d exits, thread->local_id, thread->global_id);
                goto exit_thread;
                break;
            default:
                ERROR_PRINT(Invalid Command);
                break;
        }

        /* signal for completion */
        pthread_mutex_lock(&thread->command->mutex);
        thread->command->done = 1;
        pthread_cond_signal(&thread->command->cond);
        pthread_mutex_unlock(&thread->command->mutex);
        /* wait at the barrier */
        pthread_barrier_wait(&thread->barrier->barrier);
    }

exit_thread:
    pthread_mutex_lock(&thread->command->mutex);
    thread->command->done = 1;
    pthread_cond_signal(&thread->command->cond);
    pthread_mutex_unlock(&thread->command->mutex);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d has completed, thread->local_id, thread->global_id);
    pthread_exit(NULL);
    return NULL;
}

int _prepare_cmd(LikwidThreadCommand cmd, RuntimeThreadConfig* thread)
{
    int err = 0;
    if (thread == NULL)
    {   
        return -EINVAL;
    }
    
    err = pthread_mutex_lock(&thread->command->mutex);
    if (err != 0)
    {
        ERROR_PRINT(Failed to lock mutex for thread %d with error %s, thread->local_id, strerror(err));
        return err;
    }

    thread->command->cmd = cmd;
    /* Initially when threads are created it is created with done status as 1
     * once after creation, as we send the cmd - the status is set to 0
     * later invocation chnages the status to 1
     * */
    thread->command->done = 0;
    err = pthread_cond_signal(&thread->command->cond);
    if (err != 0)
    {
        ERROR_PRINT(Failed to signal condition for thread %d with error %s, thread->local_id, strerror(err));
        pthread_mutex_unlock(&thread->command->mutex);
        return err;
    }
    pthread_mutex_unlock(&thread->command->mutex);
    return err;
}

int _send_signal(RuntimeThreadConfig* thread)
{
    int err = 0;
    if (thread == NULL)
    {   
        return -EINVAL;
    }
    
    err = pthread_mutex_lock(&thread->command->mutex);
    if (err != 0)
    {
        ERROR_PRINT(Failed to lock mutex for thread %d with error %s, thread->local_id, strerror(err));
        return err;
    }

    err = pthread_cond_signal(&thread->command->cond);
    if (err != 0)
    {
        ERROR_PRINT(Failed to signal condition for thread %d with error %s, thread->local_id, strerror(err));
        pthread_mutex_unlock(&thread->command->mutex);
        return err;
    }
    pthread_mutex_unlock(&thread->command->mutex);
    return err;
}

int _wait(RuntimeThreadConfig* thread)
{
    int err = 0;
    struct timespec ts;
    if (thread == NULL)
    {   
        return -EINVAL;
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += TIMEOUT_SECONDS;
    err = pthread_mutex_lock(&thread->command->mutex);
    if (err != 0)
    {
        ERROR_PRINT(Failed to lock mutex for thread %d with error %s, thread->local_id, strerror(err));
    }

    while (!thread->command->done)
    {
        int err = pthread_cond_timedwait(&thread->command->cond, &thread->command->mutex, &ts);
        if (err == ETIMEDOUT)
        {
            ERROR_PRINT(time out waiting for thread %d to complete command, thread->local_id);
            pthread_mutex_unlock(&thread->command->mutex);
            return err;
        }
        else if (err != 0)
        {
            ERROR_PRINT(Waiting for thread %d with error %s, thread->local_id, strerror(err));
            pthread_mutex_unlock(&thread->command->mutex);
            return err;
        }
        // pthread_cond_wait(&thread->command->cond, &thread->command->mutex);
    }

    pthread_mutex_unlock(&thread->command->mutex);
    return err;
}

int send_cmd(LikwidThreadCommand cmd, RuntimeThreadConfig* thread)
{
    int err = 0;

    err = _prepare_cmd(cmd, thread);
    if (err != 0)
    {
        return err;
    }

    err = _send_signal(thread);
    if (err != 0)
    {
        return err;
    }

    if (cmd != LIKWID_THREAD_COMMAND_NOOP)
    {
        err = _wait(thread);
    }

    return err;
}

int join_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    if (num_wgroups < 0 || !wgroups)
    {
        return -1;
    }

    int err = 0;
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        for (int i = 0; i < wg->num_threads; i++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Joining thread %3d in workgroup %3d with thread ID %16ld, wg->threads[i].local_id, w, wg->threads[i].thread);
            err = pthread_join(wg->threads[i].thread, NULL);
            if (err != 0)
            {
                ERROR_PRINT(Error joining thread %3d in workgroup %3d with error %s, wg->threads[i].local_id, w, strerror(err));
                return -1;
            }
        }
    }
    return 0;
}

int create_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    if (num_wgroups < 0 || !wgroups)
    {
        return -1;
    }

    int err = 0;
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        for (int i = 0; i < wg->num_threads; i++)
        {
            RuntimeThreadConfig* thread = &wg->threads[i];
            if (pthread_create(&thread->thread, NULL, _func_t, thread))
            {
                ERROR_PRINT(Error creating thread %3d, thread->local_id);
                err = -1;
                return err;
            }
            err = _set_t_aff(thread->thread, thread->data->hwthread);
            if (err < 0)
            {
                ERROR_PRINT(Failed to set thread affinity for workgroup %3d thread %3d, w, thread->data->hwthread);
                err = -1;
                return err;
            }
            if (DEBUGLEV_DEVELOP == global_verbosity)
            {
                _print_aff(thread->thread);
            }
        }
    }
    return 0;
}

int update_threads(RuntimeConfig* runcfg)
{
    int err = 0;
    int total_threads = 0;
    TestConfigThread* t = runcfg->tcfg->threads;
    static struct tagbstring bnumthreads = bsStatic("NUM_THREADS");
    static struct tagbstring bthreadid = bsStatic("THREAD_ID");

    // printf("Num Workgroups: %d\n", runcfg->num_wgroups);
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        wg->threads = (RuntimeThreadConfig*) malloc(wg->num_threads * sizeof(RuntimeThreadConfig));
        if (!wg->threads)
        {
            ERROR_PRINT(Failed to allocate memory for threads);
            err = -ENOMEM;
            goto free;
        }

        err = pthread_barrier_init(&wg->barrier.barrier, NULL, wg->num_threads);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize barrier %s, strerror(err));
            goto free;
        }

        for (int i = 0; i < wg->num_threads; i++)
        {
            bstring bsizes = bstrcpy(t->sizes->entry[0]);
            bstring boffsets = bstrcpy(t->offsets->entry[0]);
            RuntimeWorkgroupResult t_results;
            if (bsizes != NULL && boffsets != NULL)
            {
                err = init_result(&t_results);
                if (err != 0)
                {
                    ERROR_PRINT(Unable initialize thread results);
                    return err;
                }
                bstring bthreads = bformat("%d", wg->num_threads);
                add_variable(&t_results, &bnumthreads, bthreads);
                bdestroy(bthreads);
            }
            RuntimeThreadConfig* thread = &wg->threads[i];
            thread->command = (RuntimeThreadCommand*)malloc(sizeof(RuntimeThreadCommand));
            if (!thread->command)
            {
                ERROR_PRINT(Failed to allocate memory for thread commands);
                err = -ENOMEM;
                goto free;
            }

            pthread_mutex_init(&thread->command->mutex, NULL);
            pthread_cond_init(&thread->command->cond, NULL);
            thread->command->done = 1;
            thread->command->num_streams = wg->num_streams;
            thread->command->tstreams = wg->streams;
            size_t elems;
            if (bsizes != NULL && boffsets != NULL)
            {
                for (int s = 0; s < runcfg->tcfg->num_streams; s++)
                {
                    TestConfigStream *istream = &runcfg->tcfg->streams[s];
                    for (int k = 0; k < istream->num_dims && k < istream->dims->qty; k++)
                    {
                        elems = getstreamelems(&thread->command->tstreams[s]);
                        add_value(&t_results, istream->dims->entry[k], (double)elems);
                    }
                }
            }
            /*
             * printf("tstreams dims: %d\n", thread->command->tstreams->dims);
             * for (int s = 0; s < thread->command->tstreams->dims; s++)
             *     printf("tstreams dimsizes: %d\n", thread->command->tstreams->dimsizes[s]);
             */
            thread->command->initialization = runcfg->tcfg->initialization;
            // printf("initialize bool: %d\n", runcfg->tcfg->initialization);
            thread->command->init_val = malloc(getsizeof(thread->command->tstreams->type));
            switch(thread->command->tstreams->type)
            {
                case TEST_STREAM_TYPE_SINGLE:
                    *(float*)thread->command->init_val = thread->command->tstreams->data.fval;
                    break;
                case TEST_STREAM_TYPE_DOUBLE:
                    *(double*)thread->command->init_val = thread->command->tstreams->data.dval;
                    break;
                case TEST_STREAM_TYPE_INT:
                    *(int*)thread->command->init_val = thread->command->tstreams->data.ival;
                    break;
#ifdef WITH_HALF_PRECISION
                case TEST_STREAM_TYPE_HALF:
                    *(_Float16*)thread->command->init_val = thread->command->tstreams->data.f16val;
                    break;
#endif
                case TEST_STREAM_TYPE_INT64:
                    *(int64_t*)thread->command->init_val = thread->command->tstreams->data.i64val;
                    break;
            }

            thread->num_threads = wg->num_threads;
            thread->local_id = wg->hwthreads[i];
            thread->global_id = total_threads + i;
            if (bsizes != NULL && boffsets != NULL)
            {
                bstring btid = bformat("%d", (thread->local_id % thread->num_threads));
                add_variable(&t_results, &bthreadid, btid);
                if (DEBUGLEV_DEVELOP == global_verbosity)
                {
                    printf("The hwthread %3d results are\n", thread->local_id);
                    print_result(&t_results);
                }
                replace_all(&t_results, bsizes, NULL);
                replace_all(&t_results, boffsets, NULL);
                // printf("After replace sizes: %s\n", bdata(bsizes));
                // printf("After replace offsets: %s\n", bdata(boffsets));
                double sizes_value = 0;
                double offsets_value = 0;
                err = calculator_calc(bdata(bsizes), &sizes_value);
                if (err == 0)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Calculated formula '%s' - sizes value: %lf, bdata(bsizes), sizes_value);
                }
                else
                {
                    ERROR_PRINT(Error calculating sizes for formula '%s', bdata(bsizes));
                }
                err = calculator_calc(bdata(boffsets), &offsets_value);
                if (err == 0)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Calculated formula '%s' - offsets value: %lf, bdata(boffsets), offsets_value);
                }
                else
                {
                    ERROR_PRINT(Error calculating sizes for formula '%s', bdata(boffsets));
                }
                thread->sizes = (int64_t)sizes_value;
                thread->offsets = (off_t)offsets_value;
                if ((thread->local_id % thread->num_threads) == thread->num_threads - 1)
                {
                    thread->sizes = (int64_t)(elems - thread->offsets);
                }
                bdestroy(btid);
                bdestroy(bsizes);
                bdestroy(boffsets);
            }
            destroy_result(&t_results);
            thread->runtime = 0.0;
            thread->cycles = 0;
            thread->barrier = &wg->barrier;
            // printf("Num threads: %d\n", group->num_threads);
            thread->data = (_thread_data*)malloc(sizeof(_thread_data));
            if (!thread->data)
            {
                ERROR_PRINT(Failed to allocate memory for thread data);
                err = -ENOMEM;
                goto free;
            }
            memset(thread->data, 0, sizeof(_thread_data));
            // printf("Threadid: %d\n", thread->local_id);
            thread->data->hwthread = thread->local_id;
            thread->data->flags = THREAD_DATA_THREADINIT_FLAG;
            thread->data->iters = 0;
            thread->data->cycles = 0;
            thread->data->min_runtime = 0;
            // printf("Threadid: %d\n", thread->data->hwthread);
        }

        total_threads += wg->num_threads;
    }

    return 0;

free:
    destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
    return err;
}
