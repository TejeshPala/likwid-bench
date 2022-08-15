#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <error.h>

#include "test_types.h"

#include "allocator.h"

size_t getstreamelems(RuntimeStreamConfig *sdata)
{
    size_t total = 0;
    if (sdata)
    {
        for (int i = 0; i < sdata->dims; i++) total *= (size_t)sdata->dimsizes[i];
    }
    return total;
}

size_t getstreambytes(RuntimeStreamConfig *sdata)
{
    size_t total = 0;
    if (sdata)
    {
        for (int i = 0; i < sdata->dims; i++) total *= (size_t)sdata->dimsizes[i];
        switch(sdata->type)
        {
            case TEST_STREAM_TYPE_SINGLE:
                total *= sizeof(float);
                break;
            case TEST_STREAM_TYPE_DOUBLE:
                total *= sizeof(double);
                break;
            case TEST_STREAM_TYPE_INT:
                total *= sizeof(int);
                break;
        }
    }
    return total;
}

size_t getstreamdimbytes(RuntimeStreamConfig *sdata, int dim)
{
    size_t total = 0;
    if (sdata)
    {
        if (dim >= 0 && dim < sdata->dims)
        {
            total = sdata->dimsizes[dim];
            switch(sdata->type)
            {
                case TEST_STREAM_TYPE_SINGLE:
                    total *= sizeof(float);
                    break;
                case TEST_STREAM_TYPE_DOUBLE:
                    total *= sizeof(double);
                    break;
                case TEST_STREAM_TYPE_INT:
                    total *= sizeof(int);
                    break;
            }
        }
    }
    return total;
}

int _allocate_arrays_1dim(RuntimeStreamConfig *sdata)
{
    size_t msize = getstreambytes(sdata);
    printf("allocate %lu Bytes for str %d\n", msize, sdata->id);
    void* p = malloc(msize);
    if (!p)
    {
        return ENOMEM;
    }
    sdata->ptr = p;
    return 0;
}

#define DEFINE_2DIM_TYPE_CASE_ALLOC(streamtype, datatype) \
    case streamtype: \
        msize1 = size1 * sizeof(datatype*); \
        msize2 = size2 * sizeof(datatype); \
        datatype ** p_##datatype = malloc(msize1); \
        if (!p_##datatype) \
        { \
            return ENOMEM; \
        } \
        for (int i = 0; i < size1; i++) \
        { \
            p_##datatype[i] = malloc(msize2); \
            if (!p_##datatype[i]) \
            { \
                for (int j = i-1; j>= 0; j--) \
                { \
                    if (p_##datatype[i]) free(p_##datatype[i]); \
                } \
                free(p_##datatype); \
                return ENOMEM; \
            } \
        } \
        sdata->ptr = (void*)p_##datatype; \
        break;


static int _allocate_arrays_2dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 2)
        return -EINVAL;
    size_t size1 = sdata->dimsizes[0];
    size_t size2 = sdata->dimsizes[1];
    size_t msize1, msize2;
    printf("_allocate_arrays_2dim s1 %lu s2 %lu\n", size1, size2);
    switch(sdata->type)
    {
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_DOUBLE, double)
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT, int)
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_SINGLE, float)
        default:
            fprintf(stderr, "Unknown stream type\n");
            break;
    }
    return 0;
}

int allocate_arrays(RuntimeStreamConfig *sdata)
{
    if (sdata->dims == 1 || sdata->flags & (1<<TEST_STREAM_ALLOC_TYPE_1DIM))
    {
        return _allocate_arrays_1dim(sdata);
    }
    else if (sdata->dims == 2)
    {
        return _allocate_arrays_2dim(sdata);
    }
    return -EINVAL;
}


void _release_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        printf("release str %d\n", sdata->id);
        free(sdata->ptr);
        sdata->ptr = NULL;
    }
}

#define DEFINE_2DIM_TYPE_CASE_RELEASE(streamtype, datatype) \
    case streamtype: \
        datatype##_ptr = (datatype **)sdata->ptr; \
        for (int i = 0; i < size1; i++) \
        { \
            if (datatype##_ptr[i]) free(datatype##_ptr[i]); \
        } \
        free(sdata->ptr); \
        sdata->ptr = NULL; \
        break;



void _release_arrays_2dim(RuntimeStreamConfig *sdata)
{
    
    if (sdata && sdata->ptr)
    {
        size_t size1 = sdata->dimsizes[0];
        int** int_ptr = NULL;
        double** double_ptr = NULL;
        float** float_ptr = NULL;
        printf("release str %d\n", sdata->id);
        switch(sdata->type)
        {
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_INT, int)
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_SINGLE, float)
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
}

void release_arrays(RuntimeStreamConfig *sdata)
{
    if (sdata->dims == 1 || sdata->flags & (1<<TEST_STREAM_ALLOC_TYPE_1DIM))
    {
        _release_arrays_1dim(sdata);
    }
    else if (sdata->dims == 2)
    {
        _release_arrays_2dim(sdata);
    }
    return;
}


#define DEFINE_1D_TYPE_CASE_INIT(streamtype, datatype) \
    case streamtype: \
        datatype##_ptr = (datatype *)sdata->ptr; \
        for (int i = 0; i < elems; i++) \
        { \
            state = sdata->init(datatype##_ptr, state, sdata->dims, i); \
        } \
        break;

int _initialize_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        int* int_ptr = NULL;
        double* double_ptr = NULL;
        float* float_ptr = NULL;
        int state = 0;
        size_t elems = getstreamelems(sdata);
        printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
        switch(sdata->type)
        {
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT, int)
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_SINGLE, float)
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

#define DEFINE_2D_TYPE_CASE_INIT(streamtype, datatype) \
    case streamtype: \
        datatype##_ptr = (datatype **)sdata->ptr; \
        for (int i = 0; i < sdata->dimsizes[0]; i++) \
        { \
            for (int j = 0; j < sdata->dimsizes[1]; j++) \
            { \
                state = sdata->init(datatype##_ptr, state, sdata->dims, i, j); \
            } \
        } \
        break;

int _initialize_arrays_2dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        int** int_ptr = NULL;
        double** double_ptr = NULL;
        float** float_ptr = NULL;
        int state = 0;
        size_t elems = getstreamelems(sdata);
        printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
        switch(sdata->type)
        {
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT, int)
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_SINGLE, float)
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

int initialize_arrays(RuntimeStreamConfig *sdata)
{
    if (sdata->dims == 1)
    {
        return _initialize_arrays_1dim(sdata);
    }
    else if (sdata->dims == 2)
    {
        return _initialize_arrays_2dim(sdata);
    }
    return -EINVAL;
}
