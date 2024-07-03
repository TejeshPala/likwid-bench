#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <error.h>

#include "test_types.h"

#include "allocator.h"
#include "bitmap.h"

size_t getstreamelems(RuntimeStreamConfig *sdata)
{
    size_t total = 0;
    if (sdata)
    {
        for (int i = 0; i < sdata->dims; i++)
        {
            if (sdata->dimsizes[i] > 0)
            {
                total *= (size_t)sdata->dimsizes[i];
            }
            else
            {
                return -EINVAL;
            }
        }
    }
    return total;
}

size_t getstreambytes(RuntimeStreamConfig *sdata)
{
    size_t total = 1;
    if (sdata)
    {
        if (sdata->dims > 0)
        {
            for (int i = 0; i < sdata->dims; i++)
            if (sdata->dimsizes[i] > 0)
            {
                total *= (size_t)sdata->dimsizes[i];
            }
            else
            {
                return -EINVAL;
            }
        }
        else
        {
            return -EINVAL;
        }
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
            case TEST_STREAM_TYPE_INT64:
                total *= sizeof(int64_t);
                break;
#ifdef WITH_HALF_PRECISION
            case TEST_STREAM_TYPE_HALF:
                total *= sizeof(_Float16);
                break;
#endif
            default:
                printf("Unknown Datatype\n");
                return -EINVAL;
                break;
        }
    }
    return total;
}

size_t getstreamdimbytes(RuntimeStreamConfig *sdata, int dim)
{
    size_t total = 1;
    if (sdata)
    {
        if (dim > 0 && dim < sdata->dims)
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
                case TEST_STREAM_TYPE_INT64:
                    total *= sizeof(int64_t);
                    break;
#ifdef WITH_HALF_PRECISION
                case TEST_STREAM_TYPE_HALF:
                    total *= sizeof(_Float16);
                    break;
#endif
            }
        }
        else
        {
            return -EINVAL;
        }
    }
    return total;
}

#define DEFINE_1DIM_TYPE_CASE_ALLOC(streamtype, datatype, offset) \
    case streamtype: \
        if (offset < 0 || (size_t)offset >= sdata->dimsizes[0]) return -EINVAL; \
        msize = (size + offset) * sizeof(datatype); \
        datatype * p_##datatype = malloc(msize); \
        if (!p_##datatype) return -ENOMEM; \
        sdata->ptr = (void*)((char*) p_##datatype + (offset * sizeof(datatype))); \
        break;

int _allocate_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 1 || sdata->dimsizes[0] <= 0) return -EINVAL;
    
    size_t size = getstreambytes(sdata);
    size_t msize;
    if (msize < 0) return msize;
    printf("_allocate_arrays_1dim %lu Bytes with stream name %s\n", size, bdata(sdata->name));
    switch(sdata->type)
    {
        DEFINE_1DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_DOUBLE, double, sdata->offsets[0])
        DEFINE_1DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT, int, sdata->offsets[0])
        DEFINE_1DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_SINGLE, float, sdata->offsets[0])
        DEFINE_1DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT64, int64_t, sdata->offsets[0])
#ifdef WITH_HALF_PRECISION
        DEFINE_1DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_HALF, _Float16, sdata->offsets[0])
#endif
        default:
            fprintf(stderr, "Unknown stream type\n");
            break;
    }
    return 0;
}

#define DEFINE_2DIM_TYPE_CASE_ALLOC(streamtype, datatype, offset1, offset2) \
    case streamtype: \
        if (offset1 < 0 || offset2 < 0 || (size_t)offset1 >= sdata->dimsizes[0] || (size_t)offset2 >= sdata->dimsizes[1]) return -EINVAL; \
        msize1 = (offset1 + size1) * sizeof(datatype*); \
        msize2 = (offset2 + size2) * sizeof(datatype); \
        datatype ** p_##datatype = malloc(msize1); \
        if (!p_##datatype) return -ENOMEM; \
        for (int i = 0; i < size1 + offset1; i++) \
        { \
            p_##datatype[i] = malloc(msize2); \
            if (!p_##datatype[i]) \
            { \
                for (int j = i - 1; j >= 0; j--) \
                { \
                    if (p_##datatype[i]) free(p_##datatype[i]); \
                } \
                free(p_##datatype); \
                return -ENOMEM; \
            } \
        } \
        sdata->ptr = (void*)((char**) p_##datatype + offset1); \
        for (int i = 0; i < offset1; i++) p_##datatype[i] = NULL; \
        break;


static int _allocate_arrays_2dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 2 || sdata->dimsizes[0] <= 0 || sdata->dimsizes[1] <= 0)
        return -EINVAL;
    size_t size1 = sdata->dimsizes[0];
    size_t size2 = sdata->dimsizes[1];
    size_t msize1, msize2;
    if (msize1 < 0)
    {
        return msize1;
    }
    else if (msize2 < 0)
    {
        return msize2;
    }
    printf("_allocate_arrays_2dim s1 %lu s2 %lu of %lu Bytes\n", size1, size2, getstreambytes(sdata));
    switch(sdata->type)
    {
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_DOUBLE, double, sdata->offsets[0], sdata->offsets[1])
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT, int, sdata->offsets[0], sdata->offsets[1])
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_SINGLE, float, sdata->offsets[0], sdata->offsets[1])
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT64, int64_t, sdata->offsets[0], sdata->offsets[1])
#ifdef WITH_HALF_PRECISION
        DEFINE_2DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_HALF, _Float16, sdata->offsets[0], sdata->offsets[1])
#endif
        default:
            fprintf(stderr, "Unknown stream type\n");
            break;
    }
    return 0;
}

#define DEFINE_3DIM_TYPE_CASE_ALLOC(streamtype, datatype, offset1, offset2, offset3) \
    case streamtype: \
        if (offset1 < 0 || offset2 < 0 || offset3 < 0 || \
            (size_t)offset1 >= sdata->dimsizes[0] || \
            (size_t)offset2 >= sdata->dimsizes[1] || \
            (size_t)offset3 >= sdata->dimsizes[2]) \
            { \
                return -EINVAL; \
            } \
        msize1 = (size1 + offset1) * sizeof(datatype**); \
        msize2 = (size2 + offset2) * sizeof(datatype*); \
        msize3 = (size3 + offset3) * sizeof(datatype); \
        datatype *** p_##datatype = malloc(msize1); \
        if (!p_##datatype) return -ENOMEM; \
        for (int i = 0; i < size1 + offset1; i++) \
        { \
            p_##datatype[i] = malloc(msize2); \
            if (!p_##datatype[i]) \
            { \
                for (int j = i-1; j >= 0; j--) free(p_##datatype[j]); \
                free(p_##datatype); \
                return -ENOMEM; \
            } \
            for (int j = 0; j < size2+ offset2; j++) \
            { \
                p_##datatype[i][j] = malloc(size3); \
                if (!p_##datatype[i][j]) \
                { \
                    for (int k = j - 1; k >= 0; k--) free(p_##datatype[i][k]); \
                    free(p_##datatype[i]); \
                    for (int k = i - 1; k >= 0; k--) \
                    { \
                        for (int l = 0; l < size2 + offset2; l++) free(p_##datatype[k][l]); \
                        free(p_##datatype[k]); \
                    } \
                    free(p_##datatype); \
                    return -ENOMEM; \
                } \
            } \
        } \
        sdata->ptr = (void*)((char***) p_##datatype + offset1); \
        for (int i = 0; i < offset1; i++) p_##datatype[i] = NULL; \
        break;


static int _allocate_arrays_3dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 3 || sdata->dimsizes[0] <= 0 || sdata->dimsizes[1] <= 0 || sdata->dimsizes[2] <= 0)
        return -EINVAL;
    size_t size1 = sdata->dimsizes[0];
    size_t size2 = sdata->dimsizes[1];
    size_t size3 = sdata->dimsizes[2];
    size_t msize1, msize2, msize3;
    if (msize1 < 0)
    {
        return msize1;
    }
    else if (msize2 < 0)
    {
        return msize2;
    }
    else if (msize3 < 0)
    {
        return msize3;
    }
    printf("_allocate_arrays_3dim s1 %lu s2 %lu s3 %lu of %lu Bytes\n", size1, size2, size3, getstreambytes(sdata));
    switch(sdata->type)
    {
        DEFINE_3DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_DOUBLE, double, sdata->offsets[0], sdata->offsets[1], sdata->offsets[2])
        DEFINE_3DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT, int, sdata->offsets[0], sdata->offsets[1], sdata->offsets[2])
        DEFINE_3DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_SINGLE, float, sdata->offsets[0], sdata->offsets[1], sdata->offsets[2])
        DEFINE_3DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_INT64, int64_t, sdata->offsets[0], sdata->offsets[1], sdata->offsets[2])
#ifdef WITH_HALF_PRECISION
        DEFINE_3DIM_TYPE_CASE_ALLOC(TEST_STREAM_TYPE_HALF, _Float16, sdata->offsets[0], sdata->offsets[1], sdata->offsets[2])
#endif
        default:
            fprintf(stderr, "Unknown stream type\n");
            break;
    }
    return 0;
}


int allocate_arrays(RuntimeStreamConfig *sdata)
{
    if (sdata->name == NULL) return -EINVAL;

    if (sdata->id < 0) return -EINVAL;
    
    if ((sdata->type < MIN_DATA_TYPE) || (sdata->type >= MAX_DATA_TYPE)) return -EINVAL;

    if (sdata->dims == 1 || is_bit_set(&sdata->flags, TEST_STREAM_ALLOC_TYPE_1DIM))
    {
        return _allocate_arrays_1dim(sdata);
    }
    else if (sdata->dims == 2)
    {
        return _allocate_arrays_2dim(sdata);
    }
    else if (sdata->dims == 3)
    {
        return _allocate_arrays_3dim(sdata);
    }
    return -EINVAL;
}

int allocate_streams(RuntimeConfig* runcfg)
{
    if (runcfg->tcfg->num_streams == 0 || runcfg->num_wgroups == 0)
    {
        return 0;
    }
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Allocating %d streams, runcfg->tcfg->num_streams);
        wg->streams = malloc(runcfg->tcfg->num_streams * sizeof(RuntimeStreamConfig));
        if (!wg->streams)
        {
            return -ENOMEM;
        }
        memset(wg->streams, 0, runcfg->tcfg->num_streams * sizeof(RuntimeStreamConfig));
        for (int i = 0; i < runcfg->tcfg->num_streams; i++)
        {
            TestConfigStream *istream = &runcfg->tcfg->streams[i];
            RuntimeStreamConfig* ostream = &wg->streams[i];
            if (istream && ostream)
            {
                ostream->name = bstrcpy(istream->name);
                ostream->type = istream->type;
                ostream->dims = 0;
                for (int j = 0; j < istream->num_dims && j < istream->dims->qty; j++)
                {
                    const char* t = bdata(istream->dims->entry[j]);
                    printf("dimsize %s\n",t);
                    ostream->dimsizes[j] = strtoull(t, NULL, 10);
                    ostream->dims++;
                }
            }
        }
        wg->num_streams = runcfg->tcfg->num_streams;
        for (int i = 0; i < wg->num_streams; i++)
        {
            allocate_arrays(&wg->streams[i]);
        }
    }
    return 0;
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
        tmp = sdata->ptr; \
        datatype** datatype##_ptr = (datatype **)tmp; \
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
        void* tmp = NULL;   
        printf("release str %d\n", sdata->id);
        switch(sdata->type)
        {
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_INT, int)
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_SINGLE, float)
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_INT64, int64_t)
#ifdef WITH_HALF_PRECISION
            DEFINE_2DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_HALF, _Float16)
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
}

#define DEFINE_3DIM_TYPE_CASE_RELEASE(streamtype, datatype) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype*** datatype##_ptr = (datatype ***)tmp; \
        for (int i = 0; i < size1; i++) \
        { \
            if (datatype##_ptr[i]) \
            { \
                for (int j = 0; j < size2; j++) \
                { \
                    if (datatype##_ptr[i][j]) \
                    { \
                        free(datatype##_ptr[i][j]); \
                        datatype##_ptr[i][j] = NULL; \
                    } \
                } \
                free(datatype##_ptr[i]); \
                datatype##_ptr[i] = NULL; \
            } \
        } \
        free(sdata->ptr); \
        sdata->ptr = NULL; \
        break;

void _release_arrays_3dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        size_t size1 = sdata->dimsizes[0];
        size_t size2 = sdata->dimsizes[1];
        size_t size3 = sdata->dimsizes[2];
        void* tmp = NULL;
        printf("release str %d\n", sdata->id);
        switch(sdata->type)
        {
            DEFINE_3DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_3DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_INT, int)
            DEFINE_3DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_SINGLE, float)
            DEFINE_3DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_INT64, int64_t)
#ifdef WITH_HALF_PRECISION
            DEFINE_3DIM_TYPE_CASE_RELEASE(TEST_STREAM_TYPE_HALF, _Float16)
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
}

void release_arrays(RuntimeStreamConfig *sdata)
{
    if (sdata->dims == 1 || is_bit_set(&sdata->flags, TEST_STREAM_ALLOC_TYPE_1DIM))
    {
        _release_arrays_1dim(sdata);
    }
    else if (sdata->dims == 2)
    {
        _release_arrays_2dim(sdata);
    }
    else if (sdata->dims == 3)
    {
        _release_arrays_3dim(sdata);
    }
    return;
}


#define DEFINE_1D_TYPE_CASE_INIT(streamtype, datatype) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype* datatype##_ptr = (datatype *)tmp; \
        for (int i = 0; i < elems; i++) \
        { \
            state = sdata->init(datatype##_ptr, state, sdata->dims, i); \
        } \
        break;

int _initialize_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        int state = 0;
        size_t elems = getstreamelems(sdata);
        printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
        switch(sdata->type)
        {
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT, int)
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_SINGLE, float)
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT64, int64_t)
#ifdef WITH_HALF_PRECISION
            DEFINE_1D_TYPE_CASE_INIT(TEST_STREAM_TYPE_HALF, _Float16)
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

#define DEFINE_2D_TYPE_CASE_INIT(streamtype, datatype) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype** datatype##_ptr = (datatype **)tmp; \
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
        void* tmp = NULL;
        int state = 0;
        size_t elems = getstreamelems(sdata);
        printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
        switch(sdata->type)
        {
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT, int)
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_SINGLE, float)
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT64, int64_t)
#ifdef WITH_HALF_PRECISION
            DEFINE_2D_TYPE_CASE_INIT(TEST_STREAM_TYPE_HALF, _Float16)
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

#define DEFINE_3D_TYPE_CASE_INIT(streamtype, datatype) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype*** datatype##_ptr = (datatype ***)tmp; \
        for (int i = 0; i < sdata->dimsizes[0]; i++) \
        { \
            for (int j = 0; j < sdata->dimsizes[1]; j++) \
            { \
                for (int k = 0; k < sdata->dimsizes[2]; k++) \
                { \
                    state = sdata->init(datatype##_ptr, state, sdata->dims, i, j, k); \
                } \
            } \
        } \
        break;


int _initialize_arrays_3dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        int state = 0;
        size_t elems = getstreamelems(sdata);
        printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
        switch(sdata->type)
        {
            DEFINE_3D_TYPE_CASE_INIT(TEST_STREAM_TYPE_DOUBLE, double)
            DEFINE_3D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT, int)
            DEFINE_3D_TYPE_CASE_INIT(TEST_STREAM_TYPE_SINGLE, float)
            DEFINE_3D_TYPE_CASE_INIT(TEST_STREAM_TYPE_INT64, int64_t)
#ifdef WITH_HALF_PRECISION
            DEFINE_3D_TYPE_CASE_INIT(TEST_STREAM_TYPE_HALF, _Float16)
#endif
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
    else if (sdata->dims == 3)
    {
        return _initialize_arrays_3dim(sdata);
    }
    return -EINVAL;
}
