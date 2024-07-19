#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_types.h"
#include "workgroups.h"
#include "topology.h"
#include "results.h"
#include "allocator.h"

void delete_workgroup(RuntimeWorkgroupConfig* wg)
{
    if (wg->results)
    {
        for (int l = 0; l < wg->num_threads; l++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy result storage for thread %d (HWThread %d), l, wg->hwthreads[l]);
            destroy_result(&wg->results[l]);
        }
        free(wg->results);
        wg->results = NULL;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy result storage for workgroup %s, bdata(wg->str));
        destroy_result(wg->group_results);
    }
    if (wg->hwthreads)
    {
        free(wg->hwthreads);
        wg->hwthreads = NULL;
        wg->num_threads = 0;
    }
    if (wg->str)
    {
        bdestroy(wg->str);
        wg->str = NULL;
    }
}

int resolve_workgroup(RuntimeWorkgroupConfig* wg, int maxThreads)
{
    int err = 0;
    wg->num_threads = 0;
    wg->hwthreads = malloc(maxThreads * sizeof(int));
    if (!wg->hwthreads)
    {
        ERROR_PRINT(Failed to allocate hwthread list for workgroup string %s, bdata(wg->str));
        return -ENOMEM;
    }

    int nthreads = cpustr_to_cpulist(wg->str, wg->hwthreads, maxThreads);
    if (nthreads < 0)
    {
        ERROR_PRINT(Failed to resolve workgroup string %s, bdata(wg->str));
        free(wg->hwthreads);
        wg->hwthreads = NULL;
        return nthreads;
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Workgroup string %s resolves to %d threads, bdata(wg->str), nthreads);
    wg->num_threads = nthreads;
    wg->threads = NULL;
    return 0;
}

int allocate_workgroup_stuff(RuntimeWorkgroupConfig* wg)
{
    int err = 0;
    if ((!wg) || (wg->num_threads <= 0) || (!wg->hwthreads))
    {
        return -EINVAL;
    }
    wg->results = malloc(wg->num_threads * sizeof(RuntimeWorkgroupResult));
    if (!wg->results)
    {
        ERROR_PRINT(Unable to allocate memory for results);
        return -ENOMEM;
    }
    for (int j = 0; j < wg->num_threads; j++)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Init result storage for thread %d (HWThread %d), j, wg->hwthreads[j]);
        err = init_result(&wg->results[j]);
        if (err < 0)
        {
            for (int k = 0; k < j; k++)
            {
                destroy_result(&wg->results[k]);
            }
            free(wg->results);
            wg->results = NULL;
            return err;
        }
    }
    wg->group_results = malloc(sizeof(RuntimeWorkgroupResult));
    if (!wg->group_results)
    {
        ERROR_PRINT(Unable to allocate memory for group results);
        for (int j = 0; j < wg->num_threads; j++)
        {
            destroy_result(&wg->results[j]);
        }
        free(wg->results);
        wg->results = NULL;
        return -ENOMEM;
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Init result storage for workgroup %s, bdata(wg->str));
    err = init_result(wg->group_results);
    if (err < 0)
    {
        for (int j = 0; j < wg->num_threads; j++)
        {
            destroy_result(&wg->results[j]);
        }
        free(wg->results);
        wg->results = NULL;
        free(wg->group_results);
        wg->group_results = NULL;
        return err;
    }
    return err;
}

int resolve_workgroups(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    int hwthreads = get_num_hw_threads();
    if (hwthreads <= 0)
    {
        return -EINVAL;
    }

    for (int i = 0; i < num_wgroups; i++)
    {
        int err = resolve_workgroup(&wgroups[i], hwthreads);
        if (err == 0)
        {
            err = allocate_workgroup_stuff(&wgroups[i]);
            if (err != 0)
            {
                delete_workgroup(&wgroups[i]);
                for (int j = 0; j < i; j++)
                {
                    delete_workgroup(&wgroups[j]);
                }
                return err;
            }
        }
        else
        {
            ERROR_PRINT(Unable to resolve workgroup for each workgroup);
            return err;
        }
    }
    return 0;
}

int manage_streams(RuntimeWorkgroupConfig* wg, RuntimeConfig* runcfg)
{
    int err = 0;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Allocating %d streams, runcfg->tcfg->num_streams);
    wg->streams = malloc(runcfg->tcfg->num_streams * sizeof(RuntimeStreamConfig));
    if (!wg->streams)
    {
        ERROR_PRINT(Unable to allocate memory for Streams);
        return -ENOMEM;
    }
    memset(wg->streams, 0, runcfg->tcfg->num_streams * sizeof(RuntimeStreamConfig));

    DEBUG_PRINT(DEBUGLEV_DEVELOP, Allocating %d streams, runcfg->tcfg->num_streams);
    for (int j = 0; j < runcfg->tcfg->num_streams; j++)
    {
        TestConfigStream *istream = &runcfg->tcfg->streams[j];
        RuntimeStreamConfig* ostream = &wg->streams[j];
        if (istream && ostream)
        {
            ostream->name = bstrcpy(istream->name);
            ostream->type = istream->type;
            ostream->dims = 0;
            for (int k = 0; k < istream->num_dims && k < istream->dims->qty; k++)
            {
                bstring t = bstrcpy(istream->dims->entry[k]);
                printf("dimsize before %s\n", bdata(t));
                replace_all(runcfg->global_results, t, NULL);
                int res = 0;
                int c = sscanf(bdata(t), "%d", &res);
                if (c == 1)
                {
                    ostream->dimsizes[k] = res;
                }
                printf("dimsize after %ld\n", ostream->dimsizes[k]);
                ostream->dims++;
                bdestroy(t);
            }
            // printf("name: %s, type: %d, dims: %d\n", bdata(ostream->name), ostream->type, ostream->dims);
        }
    }


    wg->num_streams = runcfg->tcfg->num_streams;
    for (int j = 0; j < wg->num_streams; j++)
    {
        err = allocate_arrays(&wg->streams[j]);
        if (err < 0)
        {
            release_arrays(&wg->streams[j]);
            for(int k = 0; k < j; k++)
            {
                release_arrays(&wg->streams[k]);
            }
            return err;
        }
    }
    return 0;
}

void _print_workgroup_cb(mpointer key, mpointer val, mpointer user_data)
{
    bstring bkey = (bstring)key;
    bstring bval = (bstring)val;
    printf("\t%s : %s\n", bdata(bkey), bdata(bval));
}

void print_workgroup(RuntimeWorkgroupConfig* wg)
{
    printf("Workgroup '%s' with %d hwthreads: ", bdata(wg->str), wg->num_threads);
    if (wg->num_threads > 0)
        printf("%d", wg->hwthreads[0]);
    for (int i = 1; i < wg->num_threads; i++)
    {
        printf(", %d", wg->hwthreads[i]);
    }
    printf("\n");
    printf("Group result\n");
    print_result(wg->group_results);
    for (int i = 0; i < wg->num_threads; i++)
    {
        printf("Results for Thread %d (HWThread %d)\n", i, wg->hwthreads[i]);
        print_result(&wg->results[i]);
    }
}

