#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mempool.h>

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

int main(int argc, char* argv[])
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
