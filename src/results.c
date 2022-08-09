#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <map.h>
#include <results.h>
#include <calculator.h>


static BenchResults* bench_results = NULL;

struct _replace_data {
    char* f;
    Map_t map;
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

static void replace(char* str, char* a, char* b)
{
    for (char *cursor=str; (cursor=strstr(cursor,a)) != NULL;)
    {
        memmove(cursor+strlen(b),cursor+strlen(a),strlen(cursor)-strlen(a)+1);
        for (int i=0; b[i]!='\0'; i++)
        {
            cursor[i] = b[i];
        }
        cursor += strlen(b);
    }
}

static void str_replace(mpointer key, mpointer value, mpointer user_data)
{
    char* k = (char*) key;
    int* ival = (int*) value;
    struct _replace_data* data = (struct _replace_data*)user_data;
    
    MapValue* val = &(data->map->values[*ival]);
    double* d = (double*)val->value;
    
    char* v = malloc(20*sizeof(char));
    int err = snprintf(v, 19, "%f", *d);
    if (err > 0)
    {
        v[err] = '\0';
        replace(data->f, k, v);
        free(v);
    }
}


int init_results(int num_threads, int *cpus)
{
    int i = 0, j = 0;
    int err = 0;
    if (!bench_results)
    {
        BenchResults* tmp = malloc(sizeof(BenchResults));
        if (tmp)
        {
            tmp->num_threads = num_threads;
            tmp->threads = malloc(num_threads * sizeof(BenchThreadResults));
            if (tmp->threads)
            {
                for (i = 0; i < tmp->num_threads; i++)
                {
                    tmp->threads[i].cpu = cpus[i];
                    err = init_smap(&tmp->threads[i].results, free);
                    printf("Init thread %d (CPU %d) -> %d\n", i, tmp->threads[i].cpu, err);
                }
            }
            else
            {
                free(tmp);
                return -ENOMEM;
            }
            err = init_smap(&tmp->formulas, free);
            if (err)
            {
                free(tmp);
                return -ENOMEM;
            }
            err = init_smap(&tmp->consts, free);
            if (err)
            {
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

int set_result(int thread, char* name, double value)
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
        double* dptr = NULL;
        err = get_smap_by_key(result->results, name, (void**)&dptr);
        if (err == -ENOENT)
        {
            dptr = (double*)double_dup(value);
            if (dptr)
            {
                printf("Adding %s (%p) value %f (%p)\n", name, name, *dptr, dptr);
                return add_smap(result->results, name, (void*)dptr);
            }
        }
        else
        {
            printf("Set %s value from %f to %f (%p)\n", name, *dptr, value, dptr);
            *dptr = value;
        }
    }
    return 0;
}

int set_result_for_all(char* name, double value)
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
                del_smap(result->results, name);
            }
            return err;
        }
    }
    return 0;
}

int add_formula(char* name, char* formula)
{
    int err = 0;
    if ((!bench_results) || (!name))
    {
        return -EINVAL;
    }
    char* f = NULL;
    err = get_smap_by_key(bench_results->formulas, name, (void**)&f);
    if (err == -ENOENT)
    {
        f = (char*)str_dup(formula);
        if (f)
        {
            return add_smap(bench_results->formulas, name, (void*)f);
        }
    }
    else
    {
        return -EEXIST;
    }
    return 0;
}

int add_const(char* name, double constant)
{
    int err = 0;
    if ((!bench_results) || (!name))
    {
        return -EINVAL;
    }
    double* f = NULL;
    err = get_smap_by_key(bench_results->consts, name, (void**)&f);
    if (err == -ENOENT)
    {
        f = (double*)double_dup(constant);
        if (f)
        {
            return add_smap(bench_results->consts, name, (void*)f);
        }
    }
    else
    {
        return -EEXIST;
    }
    return 0;
}

int get_formula(int thread, char* name, double* value)
{
    char* formula = NULL;
    struct _replace_data data;
    int err = 0;
    if (bench_results && bench_results->formulas && name && value)
    {
        err = get_smap_by_key(bench_results->formulas, name, (void**)&formula);
        if (!err)
        {

            int len = strlen(formula);
            char* tmp = malloc(len*2*sizeof(char));
            memcpy(tmp, formula, len);
            tmp[len] = '\0';
            printf("Formula: %s\n", tmp);
            data.f = tmp;
            data.map = bench_results->consts;
            foreach_in_smap(bench_results->consts, str_replace, &data);
            BenchThreadResults* result = &bench_results->threads[thread];
            data.map = result->results;
            foreach_in_smap(result->results, str_replace, &data);
            printf("Formula: %s\n", tmp);
            err = calculator_calc(tmp, value);
            printf("Result: %f Err %d\n", *value, err);
            free(tmp);
        }
    }
}

void destroy_results()
{
    if (bench_results)
    {
        for (int i = 0; i < bench_results->num_threads; i++)
        {
            BenchThreadResults* result = &bench_results->threads[i];
            destroy_smap(result->results);
            result->results = NULL;
            result->cpu = 0;
        }
        free(bench_results->threads);
        bench_results->threads = NULL;
        bench_results->num_threads = 0;
        destroy_smap(bench_results->formulas);
        bench_results->formulas = NULL;
        destroy_smap(bench_results->consts);
        bench_results->consts = NULL;
        free(bench_results);
        bench_results = NULL;
    }
}
