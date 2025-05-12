#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <error.h>

#include "test_types.h"

#include "allocator.h"
#include "bitmap.h"

uint64_t getsizeof(TestConfigStreamType type)
{
    switch(type)
    {
        case TEST_STREAM_TYPE_SINGLE:
            return sizeof(float);
        case TEST_STREAM_TYPE_DOUBLE:
            return sizeof(double);
        case TEST_STREAM_TYPE_INT:
            return sizeof(int);
        case TEST_STREAM_TYPE_INT64:
            return sizeof(int64_t);
#ifdef WITH_HALF_PRECISION
        case TEST_STREAM_TYPE_HALF:
            return sizeof(_Float16);
#endif
        default:
            fprintf(stderr, "Unknown Datatype\n");
            return -EINVAL;
    }
    return 0;
}

uint64_t getstreamelems(RuntimeStreamConfig *sdata)
{
    uint64_t total = 1;
    if (sdata)
    {
        for (uint64_t i = 0; i < sdata->dims; i++)
        {
            if (sdata->dimsizes[i] > 0)
            {
                total *= ((uint64_t)sdata->dimsizes[i] / getsizeof(sdata->type));
            }
            else
            {
                return -EINVAL;
            }
        }
    }
    return total;
}

uint64_t getstreambytes(RuntimeStreamConfig *sdata)
{
    uint64_t total = 1;
    if (sdata)
    {
        if (sdata->dims > 0)
        {
            for (uint64_t i = 0; i < sdata->dims; i++)
            {
                if (sdata->dimsizes[i] > 0)
                {
                    total *= ((uint64_t)sdata->dimsizes[i] / getsizeof(sdata->type));
                }
                else
                {
                    return -EINVAL;
                }
            }
        }
        else
        {
            return -EINVAL;
        }
    }
    return total * getsizeof(sdata->type);
}

uint64_t getstreamdimbytes(RuntimeStreamConfig *sdata, int dim)
{
    uint64_t total = 1;
    if (sdata)
    {
        if (dim > 0 && dim < sdata->dims)
        {
            total = sdata->dimsizes[dim] * getsizeof(sdata->type);
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
        if (offset < 0 || (uint64_t)offset >= sdata->dimsizes[0]) return -EINVAL; \
        uint64_t msize_##datatype = (size + offset) * sizeof(datatype); \
        if (msize_##datatype <= 0) return -EINVAL; \
        datatype * p_##datatype = NULL; \
        if (posix_memalign((void**)&p_##datatype, CL_SIZE, msize_##datatype) != 0) return -ENOMEM; \
        memset(p_##datatype, 0, msize_##datatype); \
        sdata->base_ptr = p_##datatype; \
        sdata->ptr = (char*)(p_##datatype + offset); \
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "1dim: base - %p, ptr - %p", sdata->base_ptr, sdata->ptr); \
        break;

int _allocate_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 1 || sdata->dimsizes[0] <= 0) return -EINVAL;
    
    uint64_t size = getstreamelems(sdata);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "_allocate_arrays_1dim %" PRIu64 " Bytes with stream name %s", getstreambytes(sdata), bdata(sdata->name));
    printf("Allocating 1 dimension array %s[%" PRIu64 "] - %" PRIu64 " Bytes\n", bdata(sdata->name), size, getstreambytes(sdata));
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
        if (offset1 < 0 || offset2 < 0 || (uint64_t)offset1 >= sdata->dimsizes[0] || (uint64_t)offset2 >= sdata->dimsizes[1]) return -EINVAL; \
        uint64_t msize1_##datatype, msize2_##datatype; \
        msize1_##datatype = (offset1 + size1); \
        msize2_##datatype = (offset2 + size2) * msize1_##datatype * sizeof(datatype); \
        if (msize1_##datatype <= 0 || msize2_##datatype <= 0) return -EINVAL; \
        datatype * p_##datatype = NULL; \
        if (posix_memalign((void**)&p_##datatype, CL_SIZE, msize2_##datatype) != 0) return -ENOMEM; \
        memset(p_##datatype, 0, msize2_##datatype); \
        sdata->base_ptr = p_##datatype; \
        sdata->ptr = (char*)(p_##datatype + (offset1 * (size2 + offset2) + offset2)); \
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "2dim: base - %p, ptr - %p", sdata->base_ptr, sdata->ptr); \
        break;


static int _allocate_arrays_2dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 2 || sdata->dimsizes[0] <= 0 || sdata->dimsizes[1] <= 0)
        return -EINVAL;
    uint64_t size1 = sdata->dimsizes[0] / getsizeof(sdata->type);
    uint64_t size2 = sdata->dimsizes[1] / getsizeof(sdata->type);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "_allocate_arrays_2dim s1 %" PRIu64 " s2 %" PRIu64 " of %" PRIu64 " Bytes", size1, size2, getstreambytes(sdata));
    printf("Allocating 2 dimension array %s[%" PRIu64 "][%" PRIu64 "]: %" PRIu64 " Bytes\n", bdata(sdata->name), size1, size2, getstreambytes(sdata));
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
            (uint64_t)offset1 >= sdata->dimsizes[0] || \
            (uint64_t)offset2 >= sdata->dimsizes[1] || \
            (uint64_t)offset3 >= sdata->dimsizes[2]) return -EINVAL; \
        uint64_t msize1_##datatype, msize2_##datatype, msize3_##datatype; \
        msize1_##datatype = (size1 + offset1); \
        msize2_##datatype = (msize1_##datatype) * (size2 + offset2); \
        msize3_##datatype = (msize2_##datatype) * (size3 + offset3) * sizeof(datatype); \
        if (msize1_##datatype <= 0 || msize2_##datatype <= 0 || msize3_##datatype <= 0) return -EINVAL; \
        datatype * p_##datatype = NULL; \
        if (posix_memalign((void**)&p_##datatype, CL_SIZE, msize3_##datatype) != 0) return -ENOMEM; \
        memset(p_##datatype, 0, msize3_##datatype); \
        sdata->base_ptr = p_##datatype; \
        sdata->ptr = (char*)(p_##datatype + (offset1 * (size2 + offset2) * (size3 + offset3) + offset2 * (size3 + offset3) + offset3)); \
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "3dim: base - %p, ptr - %p", sdata->base_ptr, sdata->ptr); \
        break;


static int _allocate_arrays_3dim(RuntimeStreamConfig *sdata)
{
    if (sdata->dims != 3 || sdata->dimsizes[0] <= 0 || sdata->dimsizes[1] <= 0 || sdata->dimsizes[2] <= 0)
        return -EINVAL;
    uint64_t size1 = sdata->dimsizes[0] / getsizeof(sdata->type);
    uint64_t size2 = sdata->dimsizes[1] / getsizeof(sdata->type);
    uint64_t size3 = sdata->dimsizes[2] / getsizeof(sdata->type);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "_allocate_arrays_3dim s1 %" PRIu64 " s2 %" PRIu64 " s3 %" PRIu64 " of %" PRIu64 " Bytes", size1, size2, size3, getstreambytes(sdata));
    printf("Allocating 3 dimension array %s[%" PRIu64 "][%" PRIu64 "][%" PRIu64 "]: %" PRIu64 " Bytes\n", bdata(sdata->name), size1, size2, size3, getstreambytes(sdata));
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
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Allocating %d streams", runcfg->tcfg->num_streams);
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
    if (sdata && sdata->base_ptr)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "release str %s, base ptr: %p", bdata(sdata->name), sdata->base_ptr);
        free(sdata->base_ptr);
        sdata->base_ptr = NULL;
        sdata->ptr = NULL;
    }
}

#define DEFINE_2DIM_TYPE_CASE_RELEASE(streamtype, datatype) \
    case streamtype: \
        if (sdata && sdata->base_ptr) {\
            free(sdata->base_ptr);\
            sdata->base_ptr = NULL;\
            sdata->ptr = NULL; }\
        break;



void _release_arrays_2dim(RuntimeStreamConfig *sdata)
{
    
    if (sdata && sdata->base_ptr)
    {
        uint64_t size1 = sdata->dimsizes[0];
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "release str %s, base ptr: %p", bdata(sdata->name), sdata->base_ptr);
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
        if (sdata && sdata->base_ptr) {\
            free(sdata->base_ptr); \
            sdata->ptr = NULL; \
            sdata->base_ptr = NULL; }\
        break;

void _release_arrays_3dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->base_ptr)
    {
        uint64_t size1 = sdata->dimsizes[0];
        uint64_t size2 = sdata->dimsizes[1];
        uint64_t size3 = sdata->dimsizes[2];
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "release str %s, base ptr: %p", bdata(sdata->name), sdata->base_ptr);
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


#define INIT_ARRAY(datatype) \
    do { \
        datatype * base = (datatype*) ptr; \
        if (dims == 1) base[indices[0]] = *(datatype*)init_val; \
        else if (dims == 2) \
        { \
            uint64_t stride1_##datatype = (dimsizes[1] / sizeof(datatype)); \
            base[indices[0] * stride1_##datatype + indices[1]] = *(datatype*)init_val; \
        } \
        else if (dims == 3) \
        { \
            uint64_t stride2_##datatype = (dimsizes[2] / sizeof(datatype)); \
            uint64_t stride1_##datatype = (dimsizes[1] / sizeof(datatype)) * stride2_##datatype; \
            base[indices[0] * stride1_##datatype + indices[1] * stride2_##datatype + indices[2]] = *(datatype*)init_val; \
        } \
    } while (0)


int init_function(void* ptr, int state, TestConfigStreamType type, int dims, uint64_t* dimsizes, void* init_val, ...)
{
    va_list args;
    va_start(args, init_val);
    int indices[3] = {0, 0, 0};
    for (int i = 0; i < dims; i++)
    {
        indices[i] = va_arg(args, int);
    }
    va_end(args);

    if (ptr != NULL)
    {
        switch (type)
        {
            case TEST_STREAM_TYPE_SINGLE:
                INIT_ARRAY(float);
                break;
            case TEST_STREAM_TYPE_DOUBLE:
                INIT_ARRAY(double);
                break;
            case TEST_STREAM_TYPE_INT:
                INIT_ARRAY(int);
                break;
            case TEST_STREAM_TYPE_INT64:
                INIT_ARRAY(int64_t);
                break;
#ifdef WITH_HALF_PRECISION
            case TEST_STREAM_TYPE_HALF:
                INIT_ARRAY(_Float16);
                break;
#endif
            default:
                ERROR_PRINT("Invalid datatype");
                return state;
        }
    }
    return state + 1;
}

#define DEFINE_1D_TYPE_CASE_INIT(streamtype, datatype) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype* datatype##_ptr = (datatype *)tmp; \
        for (uint64_t i = 0; i < elems; i++) \
        { \
            state = sdata->init((void*)datatype##_ptr, state, sdata->type, sdata->dims, sdata->dimsizes, sdata->init_val, i); \
        } \
        break;

int _initialize_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        int state = 0;
        uint64_t elems = getstreamelems(sdata);
        // printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
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
        datatype * datatype##_ptr = (datatype *)tmp; \
        for (uint64_t i = 0; i < size1; i++) \
        { \
            for (uint64_t j = 0; j < size2; j++) \
            { \
                state = sdata->init((void*)datatype##_ptr, state, sdata->type, sdata->dims, sdata->dimsizes, sdata->init_val, i, j);  \
            } \
        } \
        break;

int _initialize_arrays_2dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        int state = 0;
        uint64_t elems = getstreamelems(sdata);
        uint64_t size1 = sdata->dimsizes[0] / getsizeof(sdata->type);
        uint64_t size2 = sdata->dimsizes[1] / getsizeof(sdata->type);
        // printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
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
        datatype* datatype##_ptr = (datatype *)tmp; \
        for (uint64_t i = 0; i < size1; i++) \
        { \
            for (uint64_t j = 0; j < size2; j++) \
            { \
                for (uint64_t k = 0; k < size3; k++) \
                { \
                    state = sdata->init((void*)datatype##_ptr, state, sdata->type, sdata->dims, sdata->dimsizes, sdata->init_val, i, j, k); \
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
        uint64_t elems = getstreamelems(sdata);
        uint64_t size1 = sdata->dimsizes[0] / getsizeof(sdata->type);
        uint64_t size2 = sdata->dimsizes[1] / getsizeof(sdata->type);
        uint64_t size3 = sdata->dimsizes[2] / getsizeof(sdata->type);
        // printf("initialize str %d with %d dimensions and total %lu elements\n", sdata->id, sdata->dims, elems);
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


#define DEFINE_1D_TYPE_CASE_PRINT(streamtype, datatype, format) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype* datatype##_ptr = (datatype *)tmp; \
        for (uint64_t i = 0; i < elems; i++) \
        { \
            printf(format " ", (datatype)datatype##_ptr[i]); \
        } \
        printf("\n"); \
        break;

int _print_arrays_1dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        uint64_t elems = getstreamelems(sdata);
        double size = (double)getstreambytes(sdata) / 1024LL;
        printf("printing 1 dimensional array of size %lfkb with %ld elements\n", size, elems);
        switch(sdata->type)
        {
            DEFINE_1D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_DOUBLE, double, "%lf")
            DEFINE_1D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_INT, int, "%d")
            DEFINE_1D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_SINGLE, float, "%f")
            DEFINE_1D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_INT64, int64_t, "%ld")
#ifdef WITH_HALF_PRECISION
            DEFINE_1D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_HALF, _Float16, "%f")
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

#define DEFINE_2D_TYPE_CASE_PRINT(streamtype, datatype, format) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype* datatype##_ptr = (datatype *)tmp; \
        for (uint64_t i = 0; i < size1; i++) \
        { \
            for (uint64_t j = 0; j < size2; j++) \
            { \
                printf(format " ", (datatype)datatype##_ptr[i * size2 + j]); \
            } \
            printf("\n"); \
        } \
        break;

int _print_arrays_2dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        uint64_t elems = getstreamelems(sdata);
        uint64_t size1 = sdata->dimsizes[0] / getsizeof(sdata->type);
        uint64_t size2 = sdata->dimsizes[1] / getsizeof(sdata->type);
        double size = (double)getstreambytes(sdata) / 1024LL;
        printf("printing 2 dimensional array of size %lfkb with %ld elements\n", size, elems);
        switch(sdata->type)
        {
            DEFINE_2D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_DOUBLE, double, "%lf")
            DEFINE_2D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_INT, int, "%d")
            DEFINE_2D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_SINGLE, float, "%f")
            DEFINE_2D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_INT64, int64_t, "%ld")
#ifdef WITH_HALF_PRECISION
            DEFINE_2D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_HALF, _Float16, "%f")
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

#define DEFINE_3D_TYPE_CASE_PRINT(streamtype, datatype, format) \
    case streamtype: \
        tmp = sdata->ptr; \
        datatype* datatype##_ptr = (datatype *)tmp; \
        for (uint64_t i = 0; i < size1; i++) \
        { \
            printf("Layer %" PRIu64 ":\n", i); \
            for (uint64_t j = 0; j < size2; j++) \
            { \
                for (uint64_t k = 0; k < size3; k++) \
                { \
                    printf(format " ", (datatype)datatype##_ptr[i * size2 * size3 + j * size3 + k]); \
                } \
                printf("\n"); \
            } \
            printf("\n"); \
        } \
        break;

int _print_arrays_3dim(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        void* tmp = NULL;
        uint64_t elems = getstreamelems(sdata);
        uint64_t size1 = sdata->dimsizes[0] / getsizeof(sdata->type);
        uint64_t size2 = sdata->dimsizes[1] / getsizeof(sdata->type);
        uint64_t size3 = sdata->dimsizes[2] / getsizeof(sdata->type);
        double size = (double)getstreambytes(sdata) / 1024LL;
        printf("printing 3 dimensional array of size %lfkb with %ld elements\n", size, elems);
        switch(sdata->type)
        {
            DEFINE_3D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_DOUBLE, double, "%lf")
            DEFINE_3D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_INT, int, "%d")
            DEFINE_3D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_SINGLE, float, "%f")
            DEFINE_3D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_INT64, int64_t, "%ld")
#ifdef WITH_HALF_PRECISION
            DEFINE_3D_TYPE_CASE_PRINT(TEST_STREAM_TYPE_HALF, _Float16, "%f")
#endif
            default:
                fprintf(stderr, "Unknown stream type\n");
                break;
        }
    }
    return 0;
}

int print_arrays(RuntimeStreamConfig *sdata)
{
    if (sdata && sdata->ptr)
    {
        if (sdata->dims == 1)
        {
            return _print_arrays_1dim(sdata);
        }
        else if (sdata->dims == 2)
        {
            return _print_arrays_2dim(sdata);
        }
        else if (sdata->dims == 3)
        {
            return _print_arrays_3dim(sdata);
        }
    }
    return -EINVAL;
}
