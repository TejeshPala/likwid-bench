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

#include "test_types.h"
#include "thread_group.h"
#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"

int destroy_tgroups(int num_wgroups, thread_group_t* thread_groups)
{
    for (int w = 0; w < num_wgroups; w++)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread groups for workgroup %d, w);
        if (thread_groups[w].barrier)
        {
            pthread_barrier_destroy(&thread_groups[w].barrier->barrier);
            free(thread_groups[w].barrier);
        }
        for (int i = 0; i < thread_groups[w].num_threads; i++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread group %d for workgroup %d, i, w);
            if (thread_groups[w].threads[i].data)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying data for thread %d, thread_groups[w].threads[i].local_id);
                free(thread_groups[w].threads[i].data);
                thread_groups[w].threads[i].data = NULL;
            }
        }
        free(thread_groups[w].threads);
    }
    free(thread_groups);
    thread_groups = NULL;
    return 0;
}

int _set_t_aff(pthread_t thread, int cpuid)
{
    int err = 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuid, &cpuset);

    err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (err != 0)
    {
        ERROR_PRINT(Error setting thread affinity %s, strerror(err));
        return -1;
    }

    return 0;
}

void _print_aff(pthread_t thread)
{
    int err = 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (err != 0)
    {
        ERROR_PRINT(Error getting thread affinity %s, strerror(err));
        return;
    }

    printf("Threadid %lu -> hwthread/affinity: ", thread);

    for (size_t t = 0; t < CPU_SETSIZE; t++)
    {
        if (CPU_ISSET(t, &cpuset))
        {
            printf("%ld ", t);
        }
    }
    printf("\n");
}

double get_time_s()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void* _func_t(void* arg)
{
    thread_group_thread_t* thread = (thread_group_thread_t*)arg;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d is running, thread->local_id, thread->global_id);
    pthread_barrier_wait(&thread->barrier->barrier);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, thread %d with global thread %d has completed, thread->local_id, thread->global_id);
    pthread_exit(NULL);
}

int join_threads(int num_wgroups, thread_group_t* thread_groups)
{
    if (num_wgroups < 0 || !thread_groups)
    {
        return -1;
    }

    int err = 0;
    for (int w = 0; w < num_wgroups; w++)
    {
        for (int i = 0; i < thread_groups[w].num_threads; i++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Joining thread %d in workgroup %d with thread ID %d, thread_groups[w].threads[i].local_id, w, thread_groups[w].threads[i].thread);
            err = pthread_join(thread_groups[w].threads[i].thread, NULL);
            if (err != 0)
            {
                ERROR_PRINT(Error joining thread %d in workgroup %d with error %s, thread_groups[w].threads[i].local_id, w, strerror(err));
                return -1;
            }
        }
    }
    return 0;
}

int update_thread_group(RuntimeConfig* runcfg, thread_group_t** thread_groups)
{
    int err = 0;
    thread_group_t* temp_groups = (thread_group_t*) malloc(runcfg->num_wgroups * sizeof(thread_group_t));
    if (!temp_groups)
    {
        ERROR_PRINT(Failed to allocate memory for thread groups);
        err = -ENOMEM;
        return err;
    }
    // printf("Num Workgroups: %d\n", runcfg->num_wgroups);
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        thread_group_t* group = &temp_groups[w];
        group->num_threads = wg->num_threads;
        // printf("Num threads: %d\n", group->num_threads);
        group->id = wg->hwthreads;
        group->threads = (thread_group_thread_t*) malloc(group->num_threads * sizeof(thread_group_thread_t));
        if (!group->threads)
        {
            ERROR_PRINT(Failed to allocate memory for threads in group);
            err = -ENOMEM;
            goto free;
        }

        group->barrier = (thread_barrier_t*) malloc(sizeof(thread_barrier_t));
        if (!group->barrier)
        {
            ERROR_PRINT(Failed to allocate memory for barrier);
            err = -ENOMEM;
            goto free;
        }

        err = pthread_barrier_init(&group->barrier->barrier, NULL, group->num_threads);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize barrier %s, strerror(err));
            goto free;
        }

        for (int i = 0; i < group->num_threads; i++)
        {
            thread_group_thread_t* thread = &group->threads[i];
            thread->local_id = group->id[i];
            thread->global_id = w * group->num_threads + i;
            thread->runtime = 0.0;
            thread->cycles = 0;
            thread->barrier = group->barrier;
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
            thread->data->flags = 0;
            thread->data->iters = 0;
            thread->data->cycles = 0;
            thread->data->min_runtime = 0;
            // printf("Threadid: %d\n", thread->data->hwthread);
            if (pthread_create(&thread->thread, NULL, _func_t, thread))
            {
                ERROR_PRINT(Error creating thread %d, thread->local_id);
                err = -1;
                goto free;
            }
            err = _set_t_aff(thread->thread, thread->data->hwthread);
            if (err < 0)
            {
                ERROR_PRINT(Failed to set thread affinity for group %d thread %d, w, thread->data->hwthread);
                err = -1;
                goto free;
            }
            if (DEBUGLEV_DEVELOP)
            {
                _print_aff(thread->thread);
            }
        }
    }

    *thread_groups = temp_groups;
    return 0;

free:
    if (temp_groups)
    {
        for (int w = 0; w < runcfg->num_wgroups; w++)
        {
            thread_group_t* group = &temp_groups[w];
            if (group->barrier)
            {
                pthread_barrier_destroy(&group->barrier->barrier);
                free(group->barrier);
            }
            if (group->threads)
            {
                for (int i = 0; i < group->num_threads; i++)
                {
                    if (group->threads[i].data)
                    {
                        free(group->threads[i].data);
                    }
                }
                free(group->threads);
            }
        }
        free(temp_groups);
        temp_groups = NULL;
    }
    return err;
}

double bench(int (*fn)(int num_wgroups, thread_group_t* thread_groups), int num_wgroups, thread_group_t* thread_groups, RuntimeConfig* runcfg)
{
    double s = 0.0;
    double e = 0.0;
    double runtime = 0.0;
    int iter = 1;
    
    if (runcfg->runtime > 0)
    {
        do
        {
            s = get_time_s();
            for (int k = 0; k < iter; ++k)
            {
                fn(num_wgroups, thread_groups);
            }
            e = get_time_s();
            runtime = e - s;
            iter *= 2;
        } while (runtime < runcfg->runtime);
        iter /= 2;
    }

    if (runcfg->iterations > 0)
    {
        iter = (runcfg->iterations > MIN_ITERATIONS) ? runcfg->iterations : MIN_ITERATIONS;
        while (1)
        {
            s = get_time_s();
            for (int k = 0; k < iter; ++k)
            {
                fn(num_wgroups, thread_groups);
            }
            e = get_time_s();
            runtime = e - s;
            if (runtime < MIN_RUNTIME)
            {
                iter *= 2;
            }
            else
            {
                break;
            }
        }
    }

    return runtime / iter;
}
