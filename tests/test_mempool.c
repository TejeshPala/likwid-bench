#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mempool.h>
#include <time.h>

typedef struct {
    char* name;
    int (*test)();
    int error;
} TestMemPoolConfig;


int test_open_close()
{
    MemPool_t pool;
    int err = mempool_init(&pool);
    if (err != 0) {return err;};
    mempool_close(pool);
    return 0;
}

int test_open_alloc_close()
{
    MemPool_t pool;
    int err = mempool_init(&pool);
    if (err != 0) {return err;};
    void* ptr = NULL;
    err = mempool_alloc(pool, 100, &ptr);
    mempool_close(pool);
    return err;
}

int test_open_alloc_free_close()
{
    MemPool_t pool;
    int err = mempool_init(&pool);
    if (err != 0) {return err;};
    void* ptr = NULL;
    err = mempool_alloc(pool, 100, &ptr);
    if (err == 0)
    {
        err = mempool_free(pool, ptr);
    }
    mempool_close(pool);
    return err;
}

int test_open_alloc_alloc_free_close()
{
    MemPool_t pool;
    int err = mempool_init(&pool);
    if (err != 0) {return err;};
    void* ptr1 = NULL;
    void* ptr2 = NULL;
    err = mempool_alloc(pool, 100, &ptr1);
    if (err == 0)
    {
        err = mempool_free(pool, ptr1);
    }
    err = mempool_alloc(pool, 100, &ptr2);
    mempool_close(pool);
    return err;
}

int test_null_close()
{
    MemPool_t pool = NULL;
    mempool_close(pool);
    return 0;
}

int test_invalid_free()
{
    MemPool_t pool = NULL;
    int err = mempool_init(&pool);
    if (err != 0) {return err;};
    void* ptr = (void*)123;
    err = mempool_free(pool, ptr);
    if (err == 0)
    {
        err = -EINVAL;
    }
    mempool_close(pool);
    return err;
}

int test_alloc_free_mix()
{
    MemPool_t pool = NULL;
    int num_ptrs = 20;
    void** ptrs = malloc(num_ptrs * sizeof(void*));
    if (!ptrs)
    {
        return -ENOMEM;
    }
    int max_buf = (num_ptrs/2 >= 1 ? num_ptrs/2 : 1);
    int err = mempool_init_opts(&pool, max_buf, MEMPOOL_DEFAULT_INC_FACTOR);
    if (err != 0) {return err;};
    err = mempool_alloc(pool, 100, &ptrs[0]);
    if (err != 0)
    {
        mempool_close(pool);
        free(ptrs);
        return err;
    }
    for (int i = 1; i < num_ptrs; i++)
    {
        err = mempool_alloc(pool, 100, &ptrs[i]);
        if (err != 0)
        {
            break;
        }
    }
    for (int i = num_ptrs/2; i < num_ptrs; i++)
    {
        err = mempool_free(pool, ptrs[i]);
        if (err != 0)
        {
            break;
        }
    }
    
    for (int i = num_ptrs/2; i < num_ptrs; i++)
    {
        err = mempool_alloc(pool, 100, &ptrs[i]);
        if (err != 0)
        {
            break;
        }
    }
    if (err == 0)
    {
        err = mempool_free(pool, ptrs[0]);
    }
    if (err == 0)
    {
        void* last_ptr = NULL;
        err = mempool_alloc(pool, 100, &last_ptr);
    }
    free(ptrs);
    mempool_close(pool);
    return err;
}


static TestMemPoolConfig test_config[] = {

    {"open_close", test_open_close, 0},
    {"open_alloc_close", test_open_alloc_close, 0},
    {"open_alloc_free_close", test_open_alloc_free_close, 0},
    {"open_alloc_alloc_free_close", test_open_alloc_alloc_free_close, 0},
    {"null_close", test_null_close, 0},
    {"null_close", test_null_close, 0},
    {"invalid_free", test_invalid_free, -EEXIST},
    {"alloc_free_mix", test_alloc_free_mix, 0},
    {NULL, NULL},
};



int benchmark_mempool_only_alloc()
{
    size_t alloc_size = 512*sizeof(char);
    int num_allocs = 10000;
    struct timespec start = {0, 0}, stop = {0, 0};
    printf("benchmark_mempool_only_alloc\n");
    void** allocs = malloc(num_allocs * sizeof(void*));
    if (!allocs)
    {
        return -ENOMEM;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_allocs; i++)
    {
        allocs[i] = malloc(alloc_size);
        if (!allocs[i])
        {
            for (int j = 0; j < i; j++)
            {
                free(allocs[j]);
            }
            free(allocs);
            return -ENOMEM;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double malloc_time = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9));

    for (int i = 0; i < num_allocs; i++)
    {
        free(allocs[i]);
    }

    MemPool_t pool = NULL;
    int err = mempool_init(&pool);
    if (err < 0)
    {
        free(allocs);
        return err;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_allocs; i++)
    {
        err = mempool_alloc(pool, alloc_size, &allocs[i]);
        if (err < 0)
        {
            for (int j = 0; j < i; j++)
            {
                mempool_free(pool, allocs[j]);
            }
            free(allocs);
            return -ENOMEM;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double mempool_time = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9));
    printf("%d allocations of size %ld bytes: malloc %f mempool %f\n", num_allocs, alloc_size, malloc_time, mempool_time);
    mempool_close(pool);
    free(allocs);
    return 0;
}

int benchmark_mempool_alloc_free()
{
    int num_allocs = 10000;
    size_t alloc_size = 512*sizeof(char);
    struct timespec start = {0, 0}, stop = {0, 0};
    void* alloc = NULL;
    printf("benchmark_mempool_alloc_free\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_allocs; i++)
    {
        alloc = malloc(alloc_size);
        if (!alloc)
        {
            printf("malloc failed\n");
            return -ENOMEM;
        }
        free(alloc);
        alloc = NULL;
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double malloc_time = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9));


    MemPool_t pool = NULL;
    int err = mempool_init(&pool);
    if (err < 0)
    {
        return err;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_allocs; i++)
    {
        err = mempool_alloc(pool, alloc_size, &alloc);
        if (err < 0)
        {
            printf("mempool_alloc failed\n");
            return err;
        }
        mempool_free(pool, alloc);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double mempool_time = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9));
    printf("%d allocations/frees of size %ld bytes: malloc %f mempool %f\n", num_allocs, alloc_size, malloc_time, mempool_time);
    mempool_close(pool);
    return 0;
}

int benchmark_mempool_random()
{
    int max_size = 1000000;
    int num_randoms = 1000;
    struct timespec start = {0, 0}, stop = {0, 0}, for_rand = {0, 0};
    void* alloc = NULL;
    clock_gettime(CLOCK_MONOTONIC, &for_rand);
    srandom(max_size * num_randoms * for_rand.tv_sec);
    printf("benchmark_mempool_random\n");
    size_t  *sizes = malloc(num_randoms * sizeof(size_t));
    if (!sizes)
    {
        return -ENOMEM;
    }
    for (int i = 0; i < num_randoms; i++)
    {
        sizes[i] = random() % (max_size+1);
        if (sizes[i] > max_size)
        {
            sizes[i] = max_size;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_randoms; i++)
    {
        alloc = malloc(sizes[i] * sizeof(char));
        if (!alloc)
        {
            printf("malloc failed\n");
            return -ENOMEM;
        }
        free(alloc);
        alloc = NULL;
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double malloc_time = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9));
    start.tv_sec = 0;
    start.tv_nsec = 0;
    stop.tv_sec = 0;
    stop.tv_nsec = 0;
    MemPool_t pool = NULL;
    int err = mempool_init(&pool);
    if (err < 0)
    {
        return err;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_randoms; i++)
    {
        err = mempool_alloc(pool, sizes[i], &alloc);
        if (err < 0)
        {
            printf("mempool_alloc failed\n");
            return err;
        }
        mempool_free(pool, alloc);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double mempool_time = (double)(((double)(stop.tv_sec - start.tv_sec)) + (((double)(stop.tv_nsec - start.tv_nsec))*1E-9));
    printf("%d allocations/frees of random size: malloc %f mempool %f\n", num_randoms, malloc_time, mempool_time);
    mempool_close(pool);
    free(sizes);
    return 0;
}

int benchmark_mempool()
{
    int err = benchmark_mempool_only_alloc();
    if (err < 0)
    {
        return err;
    }
    err = benchmark_mempool_alloc_free();
    if (err < 0)
    {
        return err;
    }
    err = benchmark_mempool_random();
    if (err < 0)
    {
        return err;
    }
    return 0;
}

int test_mempool()
{
    int err = 0;
    int idx = 0;
    int all = 0;
    int success = 0;
    int shouldfail = 0;
    
    while (test_config[idx].name != NULL && test_config[idx].test != NULL)
    {
        err = test_config[idx].test();
        if (err == test_config[idx].error)
        {
            if (err == 0)
            {
                printf("mempool test '%s': success\n", test_config[idx].name);
                success++;
            }
            else
            {
                printf("mempool test '%s': shouldfail with error %d\n", test_config[idx].name, err);
                shouldfail++;
            }
        }

        all++;
        idx++;
    }
    printf("mempool test: all %d success %d shouldfail %d failed %d\n", all, success, shouldfail, all-success-shouldfail);
    return 0;
}

int main(int argc, char* argv[])
{
    int err = 0;
    int all = 0;
    int failed = 0;
    
    all++;
    err = test_mempool();
    if (err < 0)
    {
        failed++;
    }
    all++;
    err = benchmark_mempool();
    if (err < 0)
    {
        failed++;
    }
    printf("mempool suites: all %d failed %d\n", all, failed);
    return 0;
}
