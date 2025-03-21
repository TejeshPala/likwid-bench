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
#include "bench.h"
#include "dynload.h"

#if defined(_GNU_SOURCE) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 4)) && HAS_SCHEDAFFINITY
    #define USE_PTHREAD_AFFINITY 1
#else
    #define USE_PTHREAD_AFFINITY 0
#endif

#ifdef __linux__
#define gettid() syscall(SYS_gettid)
#else
#define gettid() getpid()
#endif

int destroy_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying threads for workgroup %3d, w);
        pthread_barrier_destroy(&wg->barrier.barrier);
        pthread_barrierattr_destroy(&wg->barrier.b_attr);
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
                    pthread_attr_destroy(&wg->threads[i].command->attr);
                    pthread_mutex_destroy(&wg->threads[i].command->mutex);
                    pthread_mutexattr_destroy(&wg->threads[i].command->m_attr);
                    pthread_cond_destroy(&wg->threads[i].command->cond);
                    pthread_condattr_destroy(&wg->threads[i].command->c_attr);
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying commands for thread %3d, wg->threads[i].local_id);
                    free(wg->threads[i].command);
                    wg->threads[i].command = NULL;
                }
                if (wg->threads[i].testconfig)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread test configs for thread %3d, wg->threads[i].local_id);
                    close_function(&wg->threads[i]);
                }
                if (wg->threads[i].tstreams)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread stream configs for thread %3d, wg->threads[i].local_id);
                    free(wg->threads[i].tstreams);
                    wg->threads[i].tstreams = NULL;
                }
                if (wg->threads[i].codelines)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread %3d codelines, wg->threads[i].local_id);
                    bstrListDestroy(wg->threads[i].codelines);
                }
                wg->threads[i].sdata = NULL;
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
            printf("%3lu ", t);
        }
    }
    printf("\n");

    if (DEBUGLEV_DEVELOP == global_verbosity)
    {
        printf("Threadid %16lu, Calling from CPU %d with PID %d, KTID %d, PPID %d-> hwthread/affinity: ", thread, sched_getcpu(), getpid(), (int)syscall(SYS_gettid), getppid());

        for (size_t t = 0; t < CPU_SETSIZE; t++)
        {
            if (CPU_ISSET(t, &cpuset))
            {
                printf("%3lu ", t);
            }
        }
        printf("\n");
    }
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
    for (int s = 0; s < thread->num_streams; s++)
    {
        RuntimeStreamConfig* data = &thread->sdata[s];
        RuntimeThreadStreamConfig* str = &thread->tstreams[s];
        // DEBUG_PRINT(DEBUGLEV_DEVELOP, dims: %d, sdata->dims);
        size_t elems = getstreamelems(data);
        size_t offset;
        size_t size;
        if (str->tsizes > 0 && str->toffsets >= 0)
        {
            offset = str->toffsets;
            size = str->tsizes;
        }
        else
        {
            elems = getstreamelems(data);
            offset = 0;
            size = elems;
        }
        if (thread->num_threads > 1 && (str->tsizes == 0 && str->toffsets == 0))
        {
            size_t chunk = elems / thread->num_threads;
            size_t rem_chunk = elems % thread->num_threads;
            // printf("num elems: %ld, num threads: %ld, chunk: %ld\n", getstreamelems(thread->command->tstreams), thread->num_threads, chunk);
            int local_id = thread_id % thread->num_threads;
            offset = local_id * chunk + (local_id < rem_chunk ? local_id : rem_chunk);
            size = chunk + (local_id < rem_chunk ? 1 : 0);
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d initializing stream %d with total elements: %ld offset: %ld, thread_id, s, elems, offset);
        printf("hwthread %3d initializing Stream %d Vector Length: %6ld Offset: %-ld\n", thread_id, s, elems, offset);
        data->init_val = thread->command->init_val;
        RuntimeStreamConfig tmp = *data;
        tmp.dims = data->dims;
        switch ((StreamDimension)data->dims)
        {
            case STREAM_DIM_1D:
                {
                    tmp.ptr = (char*)data->ptr + (offset * getsizeof(data->type));
                    tmp.dimsizes[0] = size * getsizeof(data->type);
                    break;
                }
            case STREAM_DIM_2D:
                {
                    size_t rows = data->dimsizes[0] / getsizeof(data->type);
                    size_t cols = data->dimsizes[1] / getsizeof(data->type);
                    size_t start_rows = offset / cols;
                    size_t end_rows = (offset + size - 1) / cols;
                    tmp.ptr = (char*)data->ptr + (start_rows * cols);
                    tmp.dimsizes[0] = (end_rows - start_rows + 1) * getsizeof(data->type);
                    tmp.dimsizes[1] = cols * getsizeof(data->type);
                    break;
                }
            case STREAM_DIM_3D:
                {
                    size_t dim1 = data->dimsizes[0] / getsizeof(data->type);
                    size_t dim2 = data->dimsizes[1] / getsizeof(data->type);
                    size_t dim3 = data->dimsizes[2] / getsizeof(data->type);
                    size_t slice = dim2 * dim3;
                    size_t start = offset / slice;
                    size_t end = (offset + size - 1) / slice;
                    tmp.ptr = (char*)data->ptr + (start * slice);
                    tmp.dimsizes[0] = (end - start + 1) * getsizeof(data->type);
                    tmp.dimsizes[1] = dim2 * getsizeof(data->type);
                    tmp.dimsizes[2] = dim3 * getsizeof(data->type);
                    break;
                }
        }
        tmp.type = data->type;
        tmp.init = init_function;
        tmp.init_val = data->init_val;

        err = initialize_arrays(&tmp);
        if (err != 0)
        {
            ERROR_PRINT(Initialization failed for stream %d, s);
        }
        // print_arrays(data);
    }

    return err;
}

int initialize_global(RuntimeThreadConfig* thread)
{
    int err = 0;
    for (int s = 0; s < thread->num_streams; s++)
    {
        RuntimeStreamConfig* data = &thread->sdata[s];
        RuntimeThreadStreamConfig* str = &thread->tstreams[s];
        data->init_val = thread->sdata[s].init_val;
        RuntimeStreamConfig tmp = *data;
        tmp.ptr = (char*)data->ptr;
        tmp.dims = data->dims;
        switch ((StreamDimension)data->dims)
        {
            case STREAM_DIM_1D:
                {
                    tmp.dimsizes[0] = data->dimsizes[0];
                    break;
                }
            case STREAM_DIM_2D:
                {
                    tmp.dimsizes[0] = data->dimsizes[0];
                    tmp.dimsizes[1] = data->dimsizes[1];
                    break;
                }
            case STREAM_DIM_3D:
                {
                    tmp.dimsizes[0] = data->dimsizes[0];
                    tmp.dimsizes[1] = data->dimsizes[1];
                    tmp.dimsizes[2] = data->dimsizes[2];
                    break;
                }
        }
        tmp.type = data->type;
        tmp.init = init_function;
        tmp.init_val = thread->command->init_val;

        err = initialize_arrays(&tmp);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize for stream %d, s);
            return err;
        }
        // print_arrays(data);
    }

    return err;
}

void* _func_t(void* arg)
{
    RuntimeThreadConfig* thread = (RuntimeThreadConfig*)arg;
    bool keep_running = true;
    // printf("Thread %3d Global Thread %3d running\n", thread->local_id, thread->global_id);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d is running, thread->local_id, thread->global_id);
    while (keep_running)
    {
        pthread_mutex_lock(&thread->command->mutex);
        while(thread->command->head == NULL)
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += TIMEOUT_SECONDS;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d is waiting for command, thread->local_id, thread->global_id);
            int err = pthread_cond_timedwait(&thread->command->cond, &thread->command->mutex, &ts);
            if (err != 0)
            {
                if (err == ETIMEDOUT)
                {
                    ERROR_PRINT(Thread %3d timedout waiting for command, thread->local_id);
                }
                else
                {
                    ERROR_PRINT(Thread %3d failed to wait for command: %s, thread->local_id, strerror(err));
                }
                pthread_mutex_unlock(&thread->command->mutex);
                goto exit_thread;
            }
            // pthread_cond_wait(&thread->command->cond, &thread->command->mutex);
        }
        Queue* q = thread->command->head;
        LikwidThreadCommand c_cmd = q->cmd;
        thread->command->head = q->next;
        if (thread->command->head == NULL)
        {
            thread->command->tail = NULL;
        }
        free(q);
        pthread_mutex_unlock(&thread->command->mutex);

        // DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d received cmd %3d, thread->local_id, thread->global_id, c_cmd);

        switch(c_cmd)
        {
            case LIKWID_THREAD_COMMAND_INITIALIZE:
                if (thread->global_id == 0 && !thread->command->initialization)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Global Initialization on thread %3d with global thread %3d, thread->local_id, thread->global_id);
                    printf("Global Initialization on thread %3d with global thread %3d\n", thread->local_id, thread->global_id);
                    int err = initialize_global(thread);
                    if (err != 0)
                    {
                        ERROR_PRINT(Global Initialization failed for thread %3d with global thread %3d, thread->local_id, thread->global_id);
                    }
                }
                else if (thread->command->initialization)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Local Initialization on thread %3d with global thread %3d, thread->local_id, thread->global_id);
                    int err = initialize_local(thread, thread->local_id);
                    if (err != 0)
                    {
                        ERROR_PRINT(Local Initialization failed for thread %3d with global thread %3d, thread->local_id, thread->global_id);
                    }
                }
                break;

            case LIKWID_THREAD_COMMAND_NOOP:
                DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d is set with NOOP command, thread->local_id, thread->global_id);
                break;

            case LIKWID_THREAD_COMMAND_RUN:
                DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d is set with RUN command, thread->local_id, thread->global_id);
                pthread_mutex_lock(&thread->command->mutex);
                thread->command->done = 1;
                pthread_cond_signal(&thread->command->cond);
                pthread_mutex_unlock(&thread->command->mutex);
                int err = run_benchmark(thread);
                if (err != 0)
                {
                    ERROR_PRINT(Running benchmark kernel failed for thread %3d with global thread %3d, thread->local_id, thread->global_id);
                }
                break;

            case LIKWID_THREAD_COMMAND_EXIT:
                DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d is set with EXIT command, thread->local_id, thread->global_id);
                keep_running = false;
                goto exit_thread;
                break;

            default:
                ERROR_PRINT(Invalid Command);
                keep_running = false;
                goto exit_thread;
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
    pthread_barrier_wait(&thread->barrier->barrier);
    pthread_mutex_lock(&thread->command->mutex);
    thread->command->done = 1;
    pthread_cond_signal(&thread->command->cond);
    pthread_mutex_unlock(&thread->command->mutex);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %3d with global thread %3d has completed, thread->local_id, thread->global_id);
    pthread_barrier_wait(&thread->barrier->barrier);
    pthread_exit(NULL);
    return NULL;
}

int send_cmd(LikwidThreadCommand cmd, RuntimeThreadConfig* thread)
{
    int err = 0;
    if (thread == NULL)
    {   
        return -EINVAL;
    }

    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (q == NULL)
    {
        ERROR_PRINT(Failed to allocate memory for New Command);
        return -ENOMEM;
    }
    q->cmd = cmd;
    q->next = NULL;

    err = pthread_mutex_lock(&thread->command->mutex);
    if (err != 0)
    {
        ERROR_PRINT(Failed to lock mutex for thread %d with error %s, thread->local_id, strerror(err));
        return err;
    }

    if (thread->command->tail == NULL)
    {
        thread->command->head = thread->command->tail = q;
    }
    else
    {
        thread->command->tail->next = q;
        thread->command->tail = q;
    }
    /* Initially when threads are created it is created with done status as 1
     * once after creation, as we send the cmd - the status is set to 0
     * later invocation changes the status to 1
     * */
    thread->command->done = 0;
    err = pthread_cond_signal(&thread->command->cond);
    if (err != 0)
    {
        ERROR_PRINT(Failed to signal condition for thread %d with error %s, thread->local_id, strerror(err));
        pthread_mutex_unlock(&thread->command->mutex);
        free(q);
        return err;
    }
    pthread_mutex_unlock(&thread->command->mutex);
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
            if (pthread_create(&thread->thread, &thread->command->attr, _func_t, thread))
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
            _print_aff(thread->thread);
        }
    }
    return 0;
}

int update_threads(RuntimeConfig* runcfg)
{
    int err = 0;
    uint64_t iter;
    int total_threads = 0;
    TestConfigStream* t = runcfg->tcfg->streams;
    static struct tagbstring bnumthreads = bsStatic("NUM_THREADS");
    static struct tagbstring bthreadid = bsStatic("THREAD_ID");
    static struct tagbstring biter = bsStatic("ITER");
    static struct tagbstring btsizes = bsStatic("sizes");
    static struct tagbstring btoffsets = bsStatic("offsets");

    if (runcfg->iterations >= 0)
    {
        iter = (runcfg->iterations > MIN_ITERATIONS) ? runcfg->iterations : MIN_ITERATIONS;
        if (runcfg->iterations < MIN_ITERATIONS)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Overwriting iterations to %d, MIN_ITERATIONS);
        }
        runcfg->iterations = iter;
        printf("The iterations updated per thread are: %d\n", runcfg->iterations);
    }
    bstring brun_iters = bformat("%ld", runcfg->iterations);
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

        err = pthread_barrierattr_init(&wg->barrier.b_attr);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize barrier attribute %s, strerror(err));
            goto free;
        }

        err = pthread_barrierattr_setpshared(&wg->barrier.b_attr, PTHREAD_PROCESS_PRIVATE);
        if (err != 0)
        {
            ERROR_PRINT(Failed to set barrier attributes %s, strerror(err));
            pthread_barrierattr_destroy(&wg->barrier.b_attr);
            goto free;
        }

        err = pthread_barrier_init(&wg->barrier.barrier, &wg->barrier.b_attr, wg->num_threads);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize barrier %s, strerror(err));
            goto free;
        }

        for (int i = 0; i < wg->num_threads; i++)
        {
            err = update_variable(&wg->results[i], &biter, brun_iters);
            if (err == 0)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Variable updated for thread %d for key %s with value %lf, i, bdata(&biter), (double)runcfg->iterations);
            }
            err = update_variable(runcfg->global_results, &biter, brun_iters);
            if (err == 0)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Variable updated in global results for key %s with value %lf, bdata(&biter), (double)runcfg->iterations);
            }
            RuntimeThreadConfig* thread = &wg->threads[i];
            thread->command = (RuntimeThreadCommand*)malloc(sizeof(RuntimeThreadCommand));
            if (!thread->command)
            {
                ERROR_PRINT(Failed to allocate memory for thread commands);
                err = -ENOMEM;
                goto free;
            }
            thread->testconfig = (RuntimeTestConfig*)malloc(sizeof(RuntimeTestConfig));
            if (!thread->testconfig)
            {
                ERROR_PRINT(Failed to allocate memory for thread test config);
                err = -ENOMEM;
                goto free;
            }

            thread->command->head = thread->command->tail = NULL;

            err = pthread_attr_init(&thread->command->attr);
            if (err != 0)
            {
                ERROR_PRINT(Failed to initialize attribute %s, strerror(err));
                goto free;
            }
            pthread_attr_setdetachstate(&thread->command->attr, PTHREAD_CREATE_JOINABLE);

            pthread_mutexattr_init(&thread->command->m_attr);
            pthread_condattr_init(&thread->command->c_attr);

            pthread_mutexattr_settype(&thread->command->m_attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_condattr_setpshared(&thread->command->c_attr, PTHREAD_PROCESS_PRIVATE);

            pthread_mutex_init(&thread->command->mutex, &thread->command->m_attr);
            pthread_cond_init(&thread->command->cond, &thread->command->c_attr);
            thread->command->done = 1;
            thread->num_streams = wg->num_streams;
            thread->sdata = wg->streams;
            thread->tstreams = (RuntimeThreadStreamConfig*)malloc(wg->num_streams * sizeof(RuntimeThreadStreamConfig));
            if (!thread->tstreams)
            {
                ERROR_PRINT(Failed to allocate memory for thread stream config);
                err = -ENOMEM;
                goto free;
            }
            /*
             * printf("tstreams dims: %d\n", thread->command->tstreams->dims);
             * for (int s = 0; s < thread->command->tstreams->dims; s++)
             *     printf("tstreams dimsizes: %d\n", thread->command->tstreams->dimsizes[s]);
             */
            thread->command->initialization = runcfg->tcfg->initialization;
            // printf("initialize bool: %d\n", runcfg->tcfg->initialization);
            thread->command->init_val = malloc(getsizeof(thread->sdata->type));
            switch(thread->sdata->type)
            {
                case TEST_STREAM_TYPE_SINGLE:
                    *(float*)thread->command->init_val = thread->sdata->data.fval;
                    break;
                case TEST_STREAM_TYPE_DOUBLE:
                    *(double*)thread->command->init_val = thread->sdata->data.dval;
                    break;
                case TEST_STREAM_TYPE_INT:
                    *(int*)thread->command->init_val = thread->sdata->data.ival;
                    break;
#ifdef WITH_HALF_PRECISION
                case TEST_STREAM_TYPE_HALF:
                    *(_Float16*)thread->command->init_val = thread->sdata->data.f16val;
                    break;
#endif
                case TEST_STREAM_TYPE_INT64:
                    *(int64_t*)thread->command->init_val = thread->sdata->data.i64val;
                    break;
            }

            thread->num_threads = wg->num_threads;
            thread->local_id = wg->hwthreads[i];
            thread->global_id = total_threads + i;

            bstring btid = bformat("%d", (thread->local_id % thread->num_threads));
            bstring bthreads = bformat("%d", wg->num_threads);
            // Iterating over the streams
            // Useful in case streams are of various dimensions and to capture each threads offsets and sizes
            // (explicitly every tstreams in thread command is stored)
            for (int s = 0; s < wg->num_streams; s++)
            {
                RuntimeThreadStreamConfig* str = &thread->tstreams[s];
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Calculations for streams%d: %s, s, bdata(thread->sdata[s].name));
                TestConfigStream *istream = &runcfg->tcfg->streams[s];
                RuntimeWorkgroupResult t_results;
                err = init_result(&t_results);
                if (err != 0)
                {
                    ERROR_PRINT(Unable initialize thread results);
                    return err;
                }
                bstring bsizes = bstrcpy(istream->sizes->entry[0]);
                bstring boffsets = bstrcpy(istream->offsets->entry[0]);
                size_t elems;
                if (bsizes != NULL && boffsets != NULL)
                {
                    for (int k = 0; k < istream->num_dims && k < istream->dims->qty; k++)
                    {
                        elems = getstreamelems(&thread->sdata[s]);
                        add_value(&t_results, istream->dims->entry[k], (double)elems);
                    }
                    add_variable(&t_results, &bnumthreads, bthreads);
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
                    double sizes_value = 0.0;
                    double offsets_value = 0.0;
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
                    str->tsizes = (int64_t)sizes_value;
                    str->toffsets = (off_t)offsets_value;
                    if ((thread->local_id % thread->num_threads) == thread->num_threads - 1)
                    {
                        str->tsizes = (int64_t)(elems - str->toffsets);
                    }
                    switch(thread->sdata[s].type)
                    {
                        case TEST_STREAM_TYPE_SINGLE:
                            str->tstream_ptr = (void*)((char*) thread->sdata[s].ptr + (str->toffsets * sizeof(float)));
                            break;
                        case TEST_STREAM_TYPE_DOUBLE:
                            str->tstream_ptr = (void*)((char*) thread->sdata[s].ptr + (str->toffsets * sizeof(double)));
                            break;
                        case TEST_STREAM_TYPE_INT:
                            str->tstream_ptr = (void*)((char*) thread->sdata[s].ptr + (str->toffsets * sizeof(int)));
                            break;
#ifdef WITH_HALF_PRECISION
                        case TEST_STREAM_TYPE_HALF:
                            str->tstream_ptr = (void*)((char*) thread->sdata[s].ptr + (str->toffsets * sizeof(_Float16)));
                            break;
#endif
                        case TEST_STREAM_TYPE_INT64:
                            str->tstream_ptr = (void*)((char*) thread->sdata[s].ptr + (str->toffsets * sizeof(int64_t)));
                            break;
                    }
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Stream Ptr for wg%d thread%d-%s: %p and offset ptr: %p, w, i, bdata(thread->sdata[s].name), thread->sdata[s].ptr, (void*)str->tstream_ptr);
                }
                bdestroy(bsizes);
                bdestroy(boffsets);
                destroy_result(&t_results);
            }
            bdestroy(bthreads);
            bdestroy(btid);
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
            thread->data->iters = iter;
            thread->data->cycles = 0;
            thread->data->min_runtime = 0;
            // printf("Threadid: %d\n", thread->data->hwthread);
        }

        total_threads += wg->num_threads;
    }
    bdestroy(brun_iters);

    return 0;

free:
    destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
    return err;
}
