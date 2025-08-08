#include <inttypes.h>
#include <stdint.h>
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
#include "table.h"
#include "test_strings.h"

#undef MAX
#define MAX(a, b) ((a > b) ? (a) : (b))


void delete_workgroup(RuntimeWorkgroupConfig* wg)
{
    if (wg->results)
    {
        for (int l = 0; l < wg->num_threads; l++)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, "Destroy result storage for thread %d (HWThread %d)", l, wg->hwthreads[l]);
            destroy_result(&wg->results[l]);
        }
        free(wg->results);
        wg->results = NULL;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Destroy result storage for workgroup %s", bdata(wg->str));
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
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Releasing workgroup %d %dd arrays for stream %s", w, wg->streams[s].dims, bdata(wg->streams[s].name));
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
        ERROR_PRINT("Failed to allocate hwthread list for workgroup string %s", bdata(wg->str));
        return -ENOMEM;
    }

    int nthreads = lb_cpustr_to_cpulist(wg->str, wg->hwthreads, maxThreads);
    if (nthreads < 0)
    {
        ERROR_PRINT("Failed to resolve workgroup string %s", bdata(wg->str));
        free(wg->hwthreads);
        wg->hwthreads = NULL;
        return nthreads;
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Workgroup string %s resolves to %d threads", bdata(wg->str), nthreads);
    wg->num_threads = nthreads;
    return 0;
}

int allocate_workgroup_stuff(int detailed, RuntimeWorkgroupConfig* wg)
{
    int err = 0;
    if ((!wg) || (wg->num_threads <= 0) || (!wg->hwthreads))
    {
        return -EINVAL;
    }

    struct tagbstring* bstats;
    int num_stats;
    if (detailed == 1)
    {
        bstats = bstats1;
        num_stats = stats1_count;
    }
    else
    {
        bstats = bstats2;
        num_stats = stats2_count;
    }

    double value = 0.0;
    wg->results = malloc(wg->num_threads * sizeof(RuntimeWorkgroupResult));
    if (!wg->results)
    {
        ERROR_PRINT("Unable to allocate memory for results");
        return -ENOMEM;
    }
    for (int j = 0; j < wg->num_threads; j++)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Init result storage for thread %d (HWThread %d)", j, wg->hwthreads[j]);
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
        ERROR_PRINT("Unable to allocate memory for group results");
        for (int j = 0; j < wg->num_threads; j++)
        {
            destroy_result(&wg->results[j]);
        }
        free(wg->results);
        wg->results = NULL;
        return -ENOMEM;
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Init result storage for workgroup %s", bdata(wg->str));
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

int resolve_workgroups(int detailed, int num_wgroups, RuntimeWorkgroupConfig* wgroups)
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
            err = allocate_workgroup_stuff(detailed, &wgroups[i]);
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
            ERROR_PRINT("Unable to resolve workgroup for each workgroup");
            return err;
        }
    }
    return 0;
}

int manage_streams(RuntimeWorkgroupConfig* wg, RuntimeConfig* runcfg)
{
    int err = 0;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Allocating %d streams", runcfg->tcfg->num_streams);
    wg->streams = malloc(runcfg->tcfg->num_streams * sizeof(RuntimeStreamConfig));
    if (!wg->streams)
    {
        ERROR_PRINT("Unable to allocate memory for Streams");
        return -ENOMEM;
    }
    memset(wg->streams, 0, runcfg->tcfg->num_streams * sizeof(RuntimeStreamConfig));

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
                DEBUG_PRINT(DEBUGLEV_DEVELOP, "Stream %d: dimsize before %s", j, bdata(t));
                replace_all(runcfg->global_results, t, NULL);
                size_t res = 0;
                int c = sscanf(bdata(t), "%zu", &res);
                if (c == 1)
                {
                    ostream->dimsizes[k] = res;
                }
                if (res == 0)
                {
                    ERROR_PRINT("Invalid dimension size %s -> %zu after conversion", bdata(istream->dims->entry[k]), res);
                    bdestroy(t);
                    return -EINVAL;
                }
                DEBUG_PRINT(DEBUGLEV_DEVELOP, "Stream %d: dimsize after %zu", j, ostream->dimsizes[k]);
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
    /*
    for (int i = 0; i < bkeys->qty; i++)
    {
        printf("%s", bdata(bkeys->entry[i]));
        bstrListPrint(bvalues[i]);
    }
    printf("\n");
    */
    int err = 0;
    static struct tagbstring bcomma = bsStatic(",");
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
                    ERROR_PRINT("Unable to add value to the results");
                }
            }
            else
            {
                ERROR_PRINT("Error calculating formula: %s", bdata(full_formula));
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
    int max_len = 0;
    // struct tagbstring mbytes = bsStatic("[MByte/s]");
    struct bstrList* tmp_tcfg_keys = bstrListCreate();
    struct bstrList* tmp_tcfg_values = bstrListCreate();
    TestConfig_t cfg = runcfg->tcfg;
    for (int i = 0; i < cfg->num_vars; i++)
    {
        TestConfigVariable* v = &cfg->vars[i];
        bstrListAdd(tmp_tcfg_keys, v->name);
        bstrListAdd(tmp_tcfg_values, v->value);
        int l = blength(v->name);
        if (l > max_len) max_len = l;
    }

    struct bstrList* bkeys = bstrListCreate();
    struct bstrList* bkeys_sorted = NULL;
    struct bstrList** bgrp_values = NULL;
    struct bstrList** bvalues = NULL;
    if (!bkeys)
    {
        ERROR_PRINT("Unable to allocate memory for keys");
        return -ENOMEM;
    }
    if (bkeys->qty == 0)
    {
        collect_keys(&wgroups[0].results[0], bkeys);
        if (bkeys->qty == 0)
        {
            ERROR_PRINT("No keys are collected from the workgroup");
            bstrListDestroy(bkeys);
            return -EINVAL;
        }
    }
    bstrListSort(bkeys, &bkeys_sorted);
    bstrListDestroy(bkeys);
    for (int i = 0; i < cfg->num_metrics; i++)
    {
        TestConfigVariable* m = &cfg->metrics[i];
        bstrListAdd(bkeys_sorted, m->name);
    }
    bgrp_values = calloc(bkeys_sorted->qty, sizeof(struct bstrList*));
    if (!bgrp_values)
    {
        ERROR_PRINT("Unable to allocate memory for group values");
        bstrListDestroy(bkeys_sorted);
        return -ENOMEM;
    }
    for (int id = 0; id < bkeys_sorted->qty; id++)
    {
        bgrp_values[id] = bstrListCreate();
        if (!bgrp_values[id])
        {
            ERROR_PRINT("Unable to allocate memory for each group values");
            for (int j = 0; j < id; j++)
            {
                bstrListDestroy(bgrp_values[j]);
            }
            free(bgrp_values);
            bstrListDestroy(bkeys_sorted);
            return -ENOMEM;
        }
    }

    /*
     * The below loops are just to update the iteration for each thread results
     * and as they were not necessary for stats reporting
     */
    struct bstrList* blist1 = bstrListCreate();
    struct bstrList* blist2 = bstrListCreate();
    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        for (int t = 0; t < wg->num_threads; t++)
        {
            RuntimeThreadConfig* thread = &wg->threads[t];
            RuntimeWorkgroupResult* result = &wg->results[t];
            if (wg->hwthreads[t] == thread->data->hwthread)
            {
                bstring val = bformat("%zu", thread->data->iters);
                err = update_variable(result, &biterations, val);
                if (err == 0)
                {
                    bstrListAdd(blist1, val);
                    bstrListAdd(blist2, val);
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Variable updated for hwthread %d for key %s with value %s", thread->data->hwthread, bdata(&biterations), bdata(val));
                }
                bdestroy(val);
            }
        }
    }
    bstrListRemoveDup(blist1);
    if (blist1->qty == 1 && biseq(blist1->entry[0], blist2->entry[0]))
    {
        printf("Iterations per thread: %s\n", bdata(blist1->entry[0]));
    }
    else
    {
        printf("Iterations per thread:");
        bstrListPrint(blist2);
    }
    bstrListDestroy(blist1);
    bstrListDestroy(blist2);

    for (int w = 0; w < num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[w];
        bvalues = calloc(bkeys_sorted->qty, sizeof(struct bstrList*));
        if (!bvalues)
        {
            ERROR_PRINT("Unable to allocate memory for values of %d workgroup", w);
            bstrListDestroy(bkeys_sorted);
            return -ENOMEM;
        }
        for (int id = 0; id < bkeys_sorted->qty; id++)
        {
            bvalues[id] = bstrListCreate();
            if (!bvalues[id])
            {
                ERROR_PRINT("Unable to allocate memory for each values");
                for (int j = 0; j < id; j++)
                {
                    bstrListDestroy(bvalues[j]);
                }
                free(bvalues);
                bstrListDestroy(bkeys_sorted);
                return -ENOMEM;
            }
        }

        for (int t = 0; t < wg->num_threads; t++)
        {
            RuntimeThreadConfig* thread = &wg->threads[t];
            RuntimeWorkgroupResult* result = &wg->results[t];
            if (wg->hwthreads[t] == thread->data->hwthread)
            {
                // values in the list should be added as per sorted keys max 3 as per stats count from test_strings header
                double values[3];
                if (runcfg->detailed == 1)
                {
                    values[0] = (double)thread->data->cycles;
                    values[1] = (double)thread->data->freq;
                    // values[2] = (double)thread->data->iters;
                    values[2] = thread->runtime;
                }
                else
                {
                    // values[0] = (double)thread->data->iters;
                    values[0] = thread->runtime;
                }
                for (int id = 0; id < bkeys_sorted->qty - cfg->num_metrics; id ++)
                {
                    double value = values[id];
                    bstring t_value = bformat("%.15lf", value);
                    // printf("t_value: %s\n", bdata(t_value));
                    err = update_value(result, bkeys_sorted->entry[id], value);
                    if (err == 0)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Value updated for hwthread %d for key %s with value %.15lf", thread->data->hwthread, bdata(bkeys_sorted->entry[id]), value);
                    }
                    bstrListAdd(bvalues[id], t_value);
                    bstrListAdd(bgrp_values[id], t_value);
                    bdestroy(t_value);
                }
                for (int i = 0; i < cfg->num_metrics; i++)
                {
                    TestConfigVariable* m = &cfg->metrics[i];
                    double val;
                    bstring bcpy = bstrcpy(m->name);
                    bstring btmp = bstrcpy(m->value);
                    // For replacing values from longest variables
                    for (int l = max_len; l >= 1; l--)
                    {
                        for (int k = 0; k < tmp_tcfg_keys->qty; k++)
                        {
                            if (blength(tmp_tcfg_keys->entry[k]) == l && binstr(btmp, 0, tmp_tcfg_keys->entry[k]) != BSTR_ERR)
                            {
                                DEBUG_PRINT(DEBUGLEV_DEVELOP, "Replacing '%s' with '%s' in '%s'", bdata(tmp_tcfg_keys->entry[k]), bdata(tmp_tcfg_values->entry[k]), bdata(btmp));
                                bfindreplace(btmp, tmp_tcfg_keys->entry[k], tmp_tcfg_values->entry[k], 0);
                            }
                        }
                    }
                    // printf("bcpy: %s\n", bdata(btmp));
                    replace_all(runcfg->global_results, btmp, NULL);
                    replace_all(result, btmp, NULL);
                    err = calculator_calc(bdata(btmp), &val);
                    if (err != 0)
                    {
                        ERROR_PRINT("Error calculating formula: %s", bdata(btmp));
                    }
                    // printf("bcpy: %s, value: %lf\n", bdata(btmp), val);
                    /*
                     * the conversion has been removed as user should know when to convert them explicity based on units they needed/required
                    if (binstrcaseless(bcpy, 0, &mbytes) != BSTR_ERR)
                    {
                        val = val * 1.0E-06;
                        add_value(result, bcpy, val);
                    }
                    else
                    {
                        add_value(result, bcpy, val);
                    }
                    */
                    add_value(result, bcpy, val);
                    bstring bval = bformat("%15lf", val);
                    bstrListAdd(bvalues[bkeys_sorted->qty + i - cfg->num_metrics], bval);
                    bstrListAdd(bgrp_values[bkeys_sorted->qty + i - cfg->num_metrics], bval);
                    bdestroy(bval);
                    bdestroy(bcpy);
                    bdestroy(btmp);
                }
            }
        }

        err = _aggregate_results(bkeys_sorted, bvalues, wg->group_results);
        if (err != 0)
        {
            ERROR_PRINT("Error in aggregation of group results for workgroup %d", w);
        }
        for (int id = 0; id < bkeys_sorted->qty; id ++)
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
    err = _aggregate_results(bkeys_sorted, bgrp_values, runcfg->global_results);
    if (err != 0)
    {
        ERROR_PRINT("Error in aggregation of global results");
    }
    for (int id = 0; id < bkeys_sorted->qty; id ++)
    {
        bstrListDestroy(bgrp_values[id]);
    }
    free(bgrp_values);
    bstrListDestroy(bkeys_sorted);
    bstrListDestroy(tmp_tcfg_keys);
    bstrListDestroy(tmp_tcfg_values);
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

int update_table(RuntimeConfig* runcfg, Table** thread, Table** wgroup, Table** global, int* max_cols, int transpose)
{
    struct bstrList* bthread_keys = bstrListCreate();
    struct bstrList* bwgroup_keys = bstrListCreate();
    struct bstrList* bglobal_keys = bstrListCreate();
    struct bstrList* bthread_keys_sorted = NULL;
    struct bstrList* bwgroup_keys_sorted = NULL;
    struct bstrList* bglobal_keys_sorted = NULL;
    Table* ithread = NULL;
    Table* iwgroup = NULL;
    Table* iglobal = NULL;

    collect_keys(&runcfg->wgroups[0].results[0], bthread_keys);
    collect_keys(&runcfg->wgroups[0].group_results[0], bwgroup_keys);
    collect_keys(runcfg->global_results, bglobal_keys);

    bstrListAdd(bthread_keys, &bthreadid);
    bstrListAdd(bthread_keys, &bthreadcpu);
    bstrListAdd(bthread_keys, &bgroupid);
    bstrListAdd(bthread_keys, &bglobalid);
    bstrListAdd(bthread_keys, &bnumthreads);
    bstrListAdd(bwgroup_keys, &bgroupid);
    bstrListAdd(bwgroup_keys, &bnumthreads);
    bstrListAdd(bglobal_keys, &bnumthreads);

    bstrListSort(bthread_keys, &bthread_keys_sorted);
    bstrListSort(bwgroup_keys, &bwgroup_keys_sorted);
    bstrListSort(bglobal_keys, &bglobal_keys_sorted);

    if (transpose != 1)
    {
        *max_cols = MAX(bthread_keys->qty, MAX(bwgroup_keys->qty, bglobal_keys->qty));
    }

    bstrListDestroy(bthread_keys);
    bstrListDestroy(bwgroup_keys);
    bstrListDestroy(bglobal_keys);

    table_create(bthread_keys_sorted, &ithread);
    table_create(bwgroup_keys_sorted, &iwgroup);
    table_create(bglobal_keys_sorted, &iglobal);

    for (int i = 0; i < runcfg->num_wgroups; i++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[i];
        for (int t = 0; t < wg->num_threads; t++)
        {
            RuntimeWorkgroupResult* result = &wg->results[t];
            struct bstrList* btmp1 = bstrListCreate();
            for (int k = 0; k < bthread_keys_sorted->qty; k++)
            {
                if (biseq(bthread_keys_sorted->entry[k], &bthreadid) || biseq(bthread_keys_sorted->entry[k], &bthreadcpu) || biseq(bthread_keys_sorted->entry[k], &bgroupid) || biseq(bthread_keys_sorted->entry[k], &bglobalid) || biseq(bthread_keys_sorted->entry[k], &bnumthreads))
                {
                    size_t val;
                    if (get_variable(result, bthread_keys_sorted->entry[k], &val) == 0)
                    {
                        bstring bval = bformat("%zu", val);
                        bstrListAdd(btmp1, bval);
                        bdestroy(bval);
                    }
                }
                else
                {
                    double val;
                    if (get_value(result, bthread_keys_sorted->entry[k], &val) == 0)
                    {
                        bstring bval = bformat("%.15lf", val);
                        bstrListAdd(btmp1, bval);
                        bdestroy(bval);
                    }
                }
            }
            table_addrow(ithread, btmp1);
            bstrListDestroy(btmp1);
        }

        struct bstrList* btmp2 = bstrListCreate();
        for (int k = 0; k < bwgroup_keys_sorted->qty; k++)
        {
            if (biseq(bwgroup_keys_sorted->entry[k], &bgroupid) || biseq(bwgroup_keys_sorted->entry[k], &bnumthreads))
            {
                size_t val;
                if (get_variable(wg->group_results, bwgroup_keys_sorted->entry[k], &val) == 0)
                {
                    bstring bval = bformat("%zu", val);
                    bstrListAdd(btmp2, bval);
                    bdestroy(bval);
                }
            }
            else
            {
                double val;
                if (get_value(wg->group_results, bwgroup_keys_sorted->entry[k], &val) == 0)
                {
                    bstring bval = bformat("%.15lf", val);
                    bstrListAdd(btmp2, bval);
                    bdestroy(bval);
                }
            }
        }
        table_addrow(iwgroup, btmp2);
        bstrListDestroy(btmp2);
    }

    struct bstrList* btmp3 = bstrListCreate();
    for (int k = 0; k < bglobal_keys_sorted->qty; k++)
    {
        if (biseq(bglobal_keys_sorted->entry[k], &bnumthreads))
        {
            size_t val;
            if (get_variable(runcfg->global_results, bglobal_keys_sorted->entry[k], &val) == 0)
            {
                bstring bval = bformat("%zu", val);
                bstrListAdd(btmp3, bval);
                bdestroy(bval);
            }
        }
        else
        {
            double val;
            if (get_value(runcfg->global_results, bglobal_keys_sorted->entry[k], &val) == 0)
            {
                bstring bval = bformat("%.15lf", val);
                bstrListAdd(btmp3, bval);
                bdestroy(bval);
            }
        }
    }
    table_addrow(iglobal, btmp3);
    bstrListDestroy(btmp3);

    if (transpose == 1)
    {
        *max_cols = MAX(ithread->rows->qty, MAX(iwgroup->rows->qty, iglobal->rows->qty));
    }

    *thread = ithread;
    *wgroup = iwgroup;
    *global = iglobal;

    bstrListDestroy(bthread_keys_sorted);
    bstrListDestroy(bwgroup_keys_sorted);
    bstrListDestroy(bglobal_keys_sorted);
}
