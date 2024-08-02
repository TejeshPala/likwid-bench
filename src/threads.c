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

#include "test_types.h"
#include "thread_group.h"
#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"

int destroy_tgroups(int num_wgroups, RuntimeThreadgroupConfig* thread_groups)
{
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeThreadgroupConfig* group = &thread_groups[w];
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread groups for workgroup %d, w);
        pthread_barrier_destroy(&group->barrier.barrier);
        for (int i = 0; i < group->num_threads; i++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread group %d for workgroup %d, i, w);
            if (group->threads[i].data)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying data for thread %d, group->threads[i].local_id);
                free(group->threads[i].data);
                group->threads[i].data = NULL;
            }
            if (group->threads[i].command)
            {
                pthread_mutex_destroy(&group->threads[i].command->mutex);
                pthread_cond_destroy(&group->threads[i].command->cond);
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying commands for thread %d, group->threads[i].local_id);
                free(group->threads[i].command);
                group->threads[i].command = NULL;
            }
        }
        free(group->threads);
    }
    free(thread_groups);
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
            case LIKWID_THREAD_COMMAND_NOOP:
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

int send_cmd(LikwidThreadCommand cmd, RuntimeThreadConfig* thread)
{
    int err = 0;
    struct timespec ts;
    int completed = 1;
    if (thread == NULL)
    {
        return -1;
    }

    /* send command to specific thread in workgroups */
    err = pthread_mutex_lock(&thread->command->mutex);
    if (err != 0)
    {
        ERROR_PRINT(Failed to lock mutex for thread %d with error %s, thread->local_id, strerror(err));
        return err;
    }

    thread->command->cmd = cmd;
    thread->command->done = 0;
    err = pthread_cond_signal(&thread->command->cond);
    if (err != 0)
    {
        ERROR_PRINT(Failed to signal condition for thread %d with error %s, thread->local_id, strerror(err));
        pthread_mutex_unlock(&thread->command->mutex);
        return err;
    }
    pthread_mutex_unlock(&thread->command->mutex);

    /* wait for all thread to complete the command */
    if (cmd != LIKWID_THREAD_COMMAND_NOOP)
    {
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
                completed = 0;
                pthread_mutex_unlock(&thread->command->mutex);
                return err;
            }
            else if (err != 0)
            {
                ERROR_PRINT(Waiting for thread %d with error %s, thread->local_id, strerror(err));
                completed = 0;
                pthread_mutex_unlock(&thread->command->mutex);
                return err;
            }
            // pthread_cond_wait(&thread->command->cond, &thread->command->mutex);
        }

        pthread_mutex_unlock(&thread->command->mutex);
    }

    return completed ? 0 : -1;
}

int join_threads(int num_wgroups, RuntimeThreadgroupConfig* thread_groups)
{
    if (num_wgroups < 0 || !thread_groups)
    {
        return -1;
    }

    int err = 0;
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeThreadgroupConfig* group = &thread_groups[w];
        for (int i = 0; i < group->num_threads; i++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Joining thread %d in workgroup %d with thread ID %ld, group->threads[i].local_id, w, group->threads[i].thread);
            err = pthread_join(group->threads[i].thread, NULL);
            if (err != 0)
            {
                ERROR_PRINT(Error joining thread %d in workgroup %d with error %s, group->threads[i].local_id, w, strerror(err));
                return -1;
            }
        }
    }
    return 0;
}

int create_threads(int num_wgroups, RuntimeThreadgroupConfig* thread_groups)
{
    if (num_wgroups < 0 || !thread_groups)
    {
        return -1;
    }

    int err = 0;
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeThreadgroupConfig* group = &thread_groups[w];
        for (int i = 0; i < group->num_threads; i++)
        {
            RuntimeThreadConfig* thread = &group->threads[i];
            if (pthread_create(&thread->thread, NULL, _func_t, thread))
            {
                ERROR_PRINT(Error creating thread %d, thread->local_id);
                err = -1;
                return err;
            }
            err = _set_t_aff(thread->thread, thread->data->hwthread);
            if (err < 0)
            {
                ERROR_PRINT(Failed to set thread affinity for group %d thread %d, w, thread->data->hwthread);
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

int update_thread_group(RuntimeConfig* runcfg, RuntimeThreadgroupConfig** thread_groups)
{
    int err = 0;
    int total_threads = 0;
    RuntimeThreadgroupConfig* temp_groups = (RuntimeThreadgroupConfig*) malloc(runcfg->num_wgroups * sizeof(RuntimeThreadgroupConfig));
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
        RuntimeThreadgroupConfig* group = &temp_groups[w];
        group->num_threads = wg->num_threads;
        // printf("Num threads: %d\n", group->num_threads);
        group->hwthreads = wg->hwthreads;
        group->threads = (RuntimeThreadConfig*) malloc(group->num_threads * sizeof(RuntimeThreadConfig));
        if (!group->threads)
        {
            ERROR_PRINT(Failed to allocate memory for threads in group);
            err = -ENOMEM;
            goto free;
        }

        err = pthread_barrier_init(&group->barrier.barrier, NULL, group->num_threads);
        if (err != 0)
        {
            ERROR_PRINT(Failed to initialize barrier %s, strerror(err));
            goto free;
        }

        for (int i = 0; i < group->num_threads; i++)
        {
            RuntimeThreadConfig* thread = &group->threads[i];
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

            thread->local_id = group->hwthreads[i];
            thread->global_id = total_threads + i;
            thread->runtime = 0.0;
            thread->cycles = 0;
            thread->barrier = &group->barrier;
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

        total_threads += group->num_threads;
    }

    *thread_groups = temp_groups;
    return 0;

free:
    if (temp_groups)
    {
        for (int w = 0; w < runcfg->num_wgroups; w++)
        {
            RuntimeThreadgroupConfig* group = &temp_groups[w];
            pthread_barrier_destroy(&group->barrier.barrier);
            if (group->threads)
            {
                for (int i = 0; i < group->num_threads; i++)
                {
                    if (group->threads[i].data)
                    {
                        free(group->threads[i].data);
                    }

                    if (group->threads[i].command)
                    {
                        pthread_mutex_destroy(&group->threads[i].command->mutex);
                        pthread_cond_destroy(&group->threads[i].command->cond);
                        free(group->threads[i].command);
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

double bench(int (*fn)(int num_wgroups, RuntimeThreadgroupConfig* thread_groups), int num_wgroups, RuntimeThreadgroupConfig* thread_groups, RuntimeConfig* runcfg)
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
        if (runcfg->iterations < MIN_ITERATIONS)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Overwriting iterations to %d, MIN_ITERATIONS);
        }
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
