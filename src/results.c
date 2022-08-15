#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef WITH_BSTRING
#define WITH_BSTRING
#endif
#include "map.h"
#include "results.h"
#include "calculator.h"
#include "bstrlib.h"
#include "error.h"


static BenchResults* bench_results = NULL;

struct _replace_data {
    bstring bf;
    Map_t map;
    Map_t exclude;
};

static void* double_dup(double d)
{
    double* m = malloc(sizeof(double));
    if (m)
    {
        *m = d;
    }
    return (void*)m;
}

static char* str_dup (const char *str)
{
    char *new_str;
    int length;

    if (str)
    {
        length = strlen(str) + 1;
        new_str = malloc(length * sizeof(char));
        memcpy (new_str, str, length);
    }
    else
    {
        new_str = NULL;
    }

    return new_str;
}


static void str_replace(mpointer key, mpointer value, mpointer user_data)
{
    char* k = (char*) key;
    int* ival = (int*) value;
    struct _replace_data* data = (struct _replace_data*)user_data;

    MapValue* val = &(data->map->values[*ival]);
    double* d = (double*)val->value;

    bstring bkey = bfromcstr(key);
    bstring bval = bformat("%f", *d);
    int ret = bfindreplace(data->bf, bkey, bval, 0);
    if (ret != BSTR_OK)
    {
        printf("Replace failed\n");
    }
    bdestroy(bval);
    bdestroy(bkey);
}

static void bstr_replace(mpointer key, mpointer value, mpointer user_data)
{
    int* ival = (int*) value;
    struct _replace_data* data = (struct _replace_data*)user_data;
    
    if (data->exclude)
    {
        double* dptr = NULL;
        int err = get_bmap_by_key(data->exclude, (bstring) key, (void**)&dptr);
        if (err == 0)
        {
            return;
        }
    }

    MapValue* val = &(data->map->values[*ival]);
    double* d = (double*)val->value;

    bstring bkey = (bstring) key;
    bstring bval = bformat("%f", *d);
    int ret = bfindreplace(data->bf, bkey, bval, 0);
    if (ret != BSTR_OK)
    {
        printf("Replace failed\n");
    }
    bdestroy(bval);
}

static void bmap_free(mpointer val)
{
    bstring bstr = (bstring)val;
    bdestroy(bstr);
}


int init_results(int num_threads, int *cpus)
{
    int i = 0, j = 0;
    int err = 0;
    if (!bench_results)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Allocating result space);
        BenchResults* tmp = malloc(sizeof(BenchResults));
        if (tmp)
        {
            tmp->num_threads = num_threads;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Allocating results for %d threads, num_threads);
            tmp->threads = malloc(num_threads * sizeof(BenchThreadResults));
            if (tmp->threads)
            {
                for (i = 0; i < tmp->num_threads; i++)
                {
                    tmp->threads[i].cpu = cpus[i];
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Creating result map for thread %d (CPU %d), i, cpus[i]);
                    err = init_bmap(&tmp->threads[i].results, free);
                    if (err != 0)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Error when creating result map for thread %d (CPU %d), i, cpus[i]);
                        for (j = 0; j < i; j++)
                        {
                            destroy_bmap(tmp->threads[j].results);
                            tmp->threads[j].results = NULL;
                            destroy_bmap(tmp->threads[j].consts);
                            tmp->threads[j].consts = NULL;
                        }
                        free(tmp->threads);
                        free(tmp);
                        return -ENOMEM;
                    }
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Creating constants map for thread %d (CPU %d), i, cpus[i]);
                    err = init_bmap(&tmp->threads[i].consts, free);
                    if (err != 0)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Error when creating constants map for thread %d (CPU %d), i, cpus[i]);
                        for (j = 0; j < i; j++)
                        {
                            destroy_bmap(tmp->threads[j].results);
                            tmp->threads[j].results = NULL;
                            destroy_bmap(tmp->threads[j].consts);
                            tmp->threads[j].consts = NULL;
                        }
                        free(tmp->threads);
                        free(tmp);
                        return -ENOMEM;
                    }
                }
            }
            else
            {
                free(tmp);
                return -ENOMEM;
            }
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Creating formula map);
            err = init_bmap(&tmp->formulas, bmap_free);
            if (err)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Error when creating formula map);
                if (tmp->threads)
                {
                    for (j = 0; j < num_threads; j++)
                    {
                        destroy_bmap(tmp->threads[j].results);
                        tmp->threads[j].results = NULL;
                        destroy_bmap(tmp->threads[j].consts);
                        tmp->threads[j].consts = NULL;
                    }
                    free(tmp->threads);
                }
                free(tmp);
                return -ENOMEM;
            }
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Creating constants map);
            err = init_bmap(&tmp->consts, free);
            if (err)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Error when creating constants map);
                destroy_bmap(tmp->formulas);
                if (tmp->threads)
                {
                    for (j = 0; j < num_threads; j++)
                    {
                        destroy_bmap(tmp->threads[j].results);
                        tmp->threads[j].results = NULL;
                        destroy_bmap(tmp->threads[j].consts);
                        tmp->threads[j].consts = NULL;
                    }
                    free(tmp->threads);
                }
                free(tmp);
                return -ENOMEM;
            }
            bench_results = tmp;
        }
        else
        {
            return -ENOMEM;
        }
    }
    calculator_init();
    return 0;
}

int set_result(int thread, bstring name, double value)
{
    int err = 0;
    if ((!bench_results) || (!name) || (thread < 0))
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Invalid arguments);
        return -EINVAL;
    }
    if (thread > bench_results->num_threads)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Invalid arguments);
        return -EINVAL;
    }
    BenchThreadResults* result = &bench_results->threads[thread];
    if (result)
    {
        double* dptr = NULL;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Searching for result %s for thread %d (CPU %d), bdata(name), thread, result->cpu);
        err = get_bmap_by_key(result->results, name, (void**)&dptr);
        if (err == -ENOENT)
        {
            dptr = (double*)double_dup(value);
            if (dptr)
            {
                //printf("Adding %s (%p) value %f (%p)\n", name, name, *dptr, dptr);
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding result %s = %f for thread %d (CPU %d), bdata(name), *dptr, thread, result->cpu);
                err = add_bmap(result->results, name, (void*)dptr);
                if (err < 0)
                {
                    ERROR_PRINT(Failed adding result %s = %f for thread %d (CPU %d), bdata(name), *dptr, thread, result->cpu);
                    return err;
                }
            }
        }
        else
        {
            //printf("Set %s value from %f to %f (%p)\n", name, *dptr, value, dptr);
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Set result %s = %f for thread %d (CPU %d), bdata(name), value, thread, result->cpu);
            *dptr = value;
        }
    }
    return 0;
}

int set_result_for_all(bstring name, double value)
{
    int i = 0, j = 0, err = 0;
    if ((!bench_results) || (!name))
    {
        return -EINVAL;
    }
    for (i = 0; i < bench_results->num_threads; i++)
    {
        err = set_result(i, name, value);
        if (err)
        {
            for (j = i-1; j >= 0; j--)
            {
                BenchThreadResults* result = &bench_results->threads[j];
                del_bmap(result->results, name);
            }
            return err;
        }
    }
    return 0;
}

int add_formula(bstring name, bstring formula)
{
    int err = 0;
    if ((!bench_results) || (!name))
    {
        return -EINVAL;
    }
    bstring f;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Searching for formula %s, bdata(name));
    err = get_bmap_by_key(bench_results->formulas, name, (void**)&f);
    if (err == -ENOENT)
    {
        f = bstrcpy(formula);
        if (f)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding formula %s = %s, bdata(name), bdata(f));
            err = add_bmap(bench_results->formulas, name, (void*)f);
            if (err < 0)
            {
                ERROR_PRINT(Failed adding formula %s = %s, bdata(name), bdata(f));
                return err;
            }
        }
    }
    else
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Formula %s already exists, bdata(name));
        return -EEXIST;
    }
    return 0;
}

int add_const(bstring name, double constant)
{
    int err = 0;
    if ((!bench_results) || (!name))
    {
        return -EINVAL;
    }
    double* f = NULL;
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Searching for constant %s, bdata(name));
    err = get_bmap_by_key(bench_results->consts, name, (void**)&f);
    if (err == -ENOENT)
    {
        f = (double*)double_dup(constant);
        if (f)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding constant %s -> %f, bdata(name), *f);
            err = add_bmap(bench_results->consts, name, (void*)f);
            if (err < 0)
            {
                ERROR_PRINT(Failed adding constant %s = %f, bdata(name), *f);
                return err;
            }
        }
    }
    else
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Constant %s already exists, bdata(name));
        return -EEXIST;
    }
    return 0;
}

int add_thread_const(int thread, bstring name, double constant)
{
    int err = 0;
    if ((!bench_results) || (!name) || (thread < 0))
    {
        return -EINVAL;
    }
    if (thread > bench_results->num_threads)
    {
        return -EINVAL;
    }
    BenchThreadResults* result = &bench_results->threads[thread];
    if (result)
    {
        double* f = NULL;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Searching for thread constant %s, bdata(name));
        err = get_bmap_by_key(result->consts, name, (void**)&f);
        if (err == -ENOENT)
        {
            f = (double*)double_dup(constant);
            if (f)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding thread constant %s -> %f, bdata(name), *f);
                err = add_bmap(result->consts, name, (void*)f);
                if (err < 0)
                {
                    ERROR_PRINT(Failed adding thread constant %s = %f, bdata(name), *f);
                    return err;
                }
            }
        }
        else
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Thread constant %s already exists, bdata(name));
            return -EEXIST;
        }
    }
    return 0;
}

int get_formula(int thread, bstring name, double* value)
{
    bstring formula;
    struct _replace_data data;
    int err = 0;
    if (bench_results && bench_results->formulas && name && value)
    {
        if (thread < 0 || thread >= bench_results->num_threads)
        {
            return -EINVAL;
        }
        BenchThreadResults* result = &bench_results->threads[thread];
        err = get_bmap_by_key(bench_results->formulas, name, (void**)&formula);
        if (!err)
        {
            
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Input formula %s, bdata(formula));
            data.bf = bstrcpy(formula);
            data.map = bench_results->consts;
            data.exclude = result->consts;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Replacing Constants);
            foreach_in_smap(bench_results->consts, bstr_replace, &data);
            DEBUG_PRINT(DEBUGLEV_DEVELOP, After constants formula %s, bdata(data.bf));
            data.map = result->consts;
            data.exclude = NULL;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Replacing Thread Constants);
            foreach_in_smap(result->consts, bstr_replace, &data);
            DEBUG_PRINT(DEBUGLEV_DEVELOP, After thread constants formula %s, bdata(data.bf));
            data.map = result->results;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Replacing Values);
            foreach_in_smap(result->results, bstr_replace, &data);
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Final formula %s, bdata(data.bf));
            err = calculator_calc(bdata(data.bf), value);
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Result: %f Err %d, *value, err);
            bdestroy(data.bf);
            return 0;
        }
        return -EEXIST;
    }
    return -EINVAL;
}

void destroy_results()
{
    if (bench_results)
    {
        for (int i = 0; i < bench_results->num_threads; i++)
        {
            BenchThreadResults* result = &bench_results->threads[i];
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy results for thread %d (CPU %d), i, result->cpu);
            destroy_bmap(result->results);
            result->results = NULL;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy constants for thread %d (CPU %d), i, result->cpu);
            destroy_bmap(result->consts);
            result->consts = NULL;
            result->cpu = 0;
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Free thread list);
        free(bench_results->threads);
        bench_results->threads = NULL;
        bench_results->num_threads = 0;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Free formula map);
        destroy_bmap(bench_results->formulas);
        bench_results->formulas = NULL;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Free constants map);
        destroy_bmap(bench_results->consts);
        bench_results->consts = NULL;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Free results);
        free(bench_results);
        bench_results = NULL;
    }
}
