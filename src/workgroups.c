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
#include "map.h"
#include "calculator.h"

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

void release_streams(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    if (wgroups && num_wgroups > 0)
    {
        for (int w = 0; w < num_wgroups; w++)
        {
            RuntimeWorkgroupConfig* wg = &wgroups[w];
            if (wg->streams && wg->num_streams > 0)
            {
                for (int s = 0; s < wg->num_streams; s++)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Releasing workgroup %d %dd arrays for stream %s, w, wg->streams[s].dims, bdata(wg->streams[s].name));
                    release_arrays(&wg->streams[s]);
                    bdestroy(wg->streams[s].name);
                }
                free(wg->streams);
                wg->streams = NULL;
                wg->num_streams = 0;
            }
        }
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
    static struct tagbstring bstats[] =
    {
        bsStatic("iters"),
        bsStatic("cycles"),
        bsStatic("time"),
    };
    int num_stats = 3;
    double value = 0.0;
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
        for (int s = 0; s < num_stats; s++)
        {
            add_value(&wg->results[j], &bstats[s], value);
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
            ostream->data = istream->data;
            ostream->dims = 0;
            for (int k = 0; k < istream->num_dims && k < istream->dims->qty; k++)
            {
                bstring t = bstrcpy(istream->dims->entry[k]);
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Stream %d: dimsize before %s, j, bdata(t));
                replace_all(runcfg->global_results, t, NULL);
                int res = 0;
                int c = sscanf(bdata(t), "%d", &res);
                if (c == 1)
                {
                    ostream->dimsizes[k] = res;
                }
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Stream %d: dimsize after %ld, j, ostream->dimsizes[k]);
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

void collect_keys_func(mpointer key, mpointer value, mpointer user_data)
{
    bstring bkey = (bstring) key;
    struct bstrList* keys = (struct bstrList*) user_data;
    bstrListAdd(keys, bkey);
}

void collect_keys(RuntimeWorkgroupResult* result, struct bstrList* sl)
{
    foreach_in_bmap(result->values, collect_keys_func, sl);
}

int _aggregate_results(struct bstrList* bkeys, struct bstrList** bvalues, RuntimeWorkgroupResult* res)
{
    int err = 0;
    static struct tagbstring bcomma = bsStatic(",");
    // printf("key: %s\n", bdata(bkey));
    for (int id = 0; id < bkeys->qty; id++)
    {
        bstring key = bstrcpy(bkeys->entry[id]);
        bstring formula = bjoin(bvalues[id], &bcomma);
        for (int a = 0; a < aggregations.num_aggregations; a++)
        {
            bstring bkey = bformat("%s[%s]", bdata(key), bdata(&aggregations.baggregations[a]));
            bstring full_formula = bformat("%s(%s)", bdata(&aggregations.baggregations[a]), bdata(formula));
            // printf("key: %s, full formula: %s, len: %d\n", bdata(bkey), bdata(full_formula), blength(full_formula));
            double calc_result;
            err = calculator_calc(bdata(full_formula), &calc_result);
            if (err == 0)
            {
                // printf("calc result: %f\n", calc_result);
                err = add_value(res, bkey, calc_result);
                if (err != 0)
                {
                    ERROR_PRINT(Unable to add value to the results);
                }
            }
            else
            {
                ERROR_PRINT(Error calculating formula: %s, bdata(full_formula));
            }
            bdestroy(full_formula);
            bdestroy(bkey);
        }
        bdestroy(formula);
        bdestroy(key);
    }
    return err;
}

int update_results(RuntimeConfig* runcfg, int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    int err = 0;
    struct bstrList* bkeys = bstrListCreate();
    struct bstrList** bgrp_values = NULL;
    struct bstrList** bvalues = NULL;
    if (!bkeys)
    {
        ERROR_PRINT(Unable to allocate memory for keys);
        return -ENOMEM;
    }
    if (bkeys->qty == 0)
    {
        collect_keys(&wgroups[0].results[0], bkeys);
        if (bkeys->qty == 0)
        {
            ERROR_PRINT(No keys are collected from the workgroup);
            bstrListDestroy(bkeys);
            return -EINVAL;
        }
    }
    bgrp_values = calloc(bkeys->qty, sizeof(struct bstrList*));
    if (!bgrp_values)
    {
        ERROR_PRINT(Unable to allocate memory for group values);
        bstrListDestroy(bkeys);
        return -ENOMEM;
    }
    for (int id = 0; id < bkeys->qty; id++)
    {
        bgrp_values[id] = bstrListCreate();
        if (!bgrp_values[id])
        {
            ERROR_PRINT(Unable to allocate memory for each group values);
            for (int j = 0; j < id; j++)
            {
                bstrListDestroy(bgrp_values[j]);
            }
            free(bgrp_values);
            bstrListDestroy(bkeys);
            return -ENOMEM;
        }
    }

    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        bvalues = calloc(bkeys->qty, sizeof(struct bstrList*));
        if (!bvalues)
        {
            ERROR_PRINT(Unable to allocate memory for values of %d workgroup, w);
            bstrListDestroy(bkeys);
            return -ENOMEM;
        }
        for (int id = 0; id < bkeys->qty; id++)
        {
            bvalues[id] = bstrListCreate();
            if (!bvalues[id])
            {
                ERROR_PRINT(Unable to allocate memory for each values);
                for (int j = 0; j < id; j++)
                {
                    bstrListDestroy(bvalues[j]);
                }
                free(bvalues);
                bstrListDestroy(bkeys);
                return -ENOMEM;
            }
        }

        for (int t = 0; t < wg->num_threads; t++)
        {
            RuntimeThreadConfig* thread = &wg->threads[t];
            RuntimeWorkgroupResult* result = &wg->results[t];
            if (wg->hwthreads[t] == thread->data->hwthread)
            {
                uint64_t values[] = {thread->data->iters, thread->data->cycles, thread->data->min_runtime};
                for (int id = 0; id < bkeys->qty; id ++)
                {
                    double value = (double)values[id];
                    bstring t_value = bformat("%lf", value);
                    // printf("t_value: %s\n", bdata(t_value));
                    // if (update_bmap(result->values, bkeys->entry[id], &value, NULL) != 0)
                    err = update_value(result, bkeys->entry[id], value);
                    if (err == 0)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Value updated for thread %d for key %s with value %lf, thread->data->hwthread, bdata(bkeys->entry[id]), value);
                    }
                    bstrListAdd(bvalues[id], t_value);
                    bstrListAdd(bgrp_values[id], t_value);
                    bdestroy(t_value);
                }
            }
        }

        err = _aggregate_results(bkeys, bvalues, wg->group_results);
        if (err != 0)
        {
            ERROR_PRINT(Error in aggregation of group results for workgroup %d, w);
        }
        for (int id = 0; id < bkeys->qty; id ++)
        {
            bstrListDestroy(bvalues[id]);
        }
        free(bvalues);
        if (DEBUGLEV_DEVELOP == global_verbosity)
        {
            printf("Below are the results after initialization of Group Results for Workgroup: %d\n", w);
            print_workgroup(wg);
        }
    }
    err = _aggregate_results(bkeys, bgrp_values, runcfg->global_results);
    if (err != 0)
    {
        ERROR_PRINT(Error in aggregation of global results);
    }
    for (int id = 0; id < bkeys->qty; id ++)
    {
        bstrListDestroy(bgrp_values[id]);
    }
    free(bgrp_values);
    bstrListDestroy(bkeys);
    return err;
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
        printf("Results for Thread %3d (HWThread %3d)\n", i, wg->hwthreads[i]);
        print_result(&wg->results[i]);
    }
}

