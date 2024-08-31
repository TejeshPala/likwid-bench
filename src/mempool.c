#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mempool.h>

static int mempool_debug = 0;

void mempool_set_debug(int value)
{
    if (value >= 0)
    {
        mempool_debug = value;
    }
}



int mempool_init_opts(MemPool_t* pool, int max_buffers, int alloc_inc_factor)
{
    if (!pool)
    {
        return -EINVAL;
    }
    if (max_buffers < 1 || alloc_inc_factor < 1)
    {
        return -EINVAL;
    }
    if (mempool_debug) printf("mempool: alloc new\n");
    MemPool_t p = malloc(sizeof(MemPool));
    if (!p)
    {
        return -ENOMEM;
    }
    p->num_allocs = 0;
    p->max_allocs = 0;
    p->num_buffers = 0;
    p->max_buffers = max_buffers;
    if (mempool_debug) printf("mempool: max_buffers %d\n", p->max_buffers);
    p->inc_factor = alloc_inc_factor;
    if (mempool_debug) printf("mempool: inc_factor %d\n", p->inc_factor);
    p->allocs = NULL;
    if (mempool_debug) printf("mempool: alloc buffers of size %d\n", p->max_buffers);
    p->buffers = malloc(p->max_buffers*sizeof(MemoryPoolAllocation));
    if (!p->buffers)
    {
        return -ENOMEM;
    }
    *pool = p;
    return 0;
}

int mempool_init(MemPool_t* pool)
{
    return mempool_init_opts(pool, MEMPOOL_DEFAULT_MAX_BUFFERS, MEMPOOL_DEFAULT_INC_FACTOR);
}

static void mempool_free_alloc(MemoryPoolAllocation* alloc)
{
    if (alloc->ptr)
    {
        free(alloc->ptr);
        alloc->ptr = NULL;
        alloc->size = 0;
        alloc->realsize = 0;
    }
}

static void mempool_copy_alloc(MemoryPoolAllocation* src, MemoryPoolAllocation* dest)
{
    dest->ptr = src->ptr;
    dest->realsize = src->realsize;
    dest->size = src->size;
}

void mempool_close(MemPool_t pool)
{
    if (!pool)
    {
        return;
    }
    if (pool->allocs)
    {
        if (mempool_debug) printf("mempool: delete %d allocations\n", pool->num_allocs);
        for (int i = 0; i < pool->num_allocs; i++)
        {
            mempool_free_alloc(&pool->allocs[i]);
        }
        free(pool->allocs);
        pool->allocs = NULL;
        pool->max_allocs = 0;
        pool->num_allocs = 0;
        pool->inc_factor = 0;
        if (mempool_debug) printf("mempool: reset allocations\n");
    }
    if (pool->buffers)
    {
        if (mempool_debug) printf("mempool: delete %d buffers\n", pool->num_buffers);
        for (int i = 0; i < pool->num_buffers; i++)
        {
            mempool_free_alloc(&pool->buffers[i]);
        }
        free(pool->buffers);
        pool->buffers = NULL;
        pool->num_buffers = 0;
        pool->max_buffers = 0;
        if (mempool_debug) printf("mempool: reset buffers\n");
    }
    if (pool)
    {
        if (mempool_debug) printf("mempool: free\n");
        free(pool);
    }
}

static int mempool_new_alloc(MemPool_t pool, MemoryPoolAllocation** alloc)
{
    if (pool->num_allocs == pool->max_allocs)
    {
        if (mempool_debug) printf("mempool: increase size of allocations from %d to %d\n", pool->max_allocs, pool->max_allocs + pool->inc_factor);
        MemoryPoolAllocation* tmp = realloc(pool->allocs, (pool->max_allocs + pool->inc_factor)*sizeof(MemoryPoolAllocation));
        if (!tmp)
        {
            return -ENOMEM;
        }
        pool->allocs = tmp;
        for (int i = pool->max_allocs; i < pool->max_allocs + pool->inc_factor; i++)
        {
            if (mempool_debug) printf("mempool: init allocation %d\n", i);
            MemoryPoolAllocation* a = &pool->allocs[i];
            a->ptr = NULL;
            a->size = 0;
            a->realsize = 0;
        }
        pool->max_allocs += pool->inc_factor;
    }
    MemoryPoolAllocation *a = &pool->allocs[pool->num_allocs];
    if (mempool_debug) printf("mempool: return allocation index %d\n", pool->num_allocs);
    a->ptr = NULL;
    a->size = 0;
    a->realsize = 0;
    *alloc = a;
    pool->num_allocs++;
    return 0;
}



int mempool_alloc(MemPool_t pool, size_t size, void** ptr)
{
    int err = 0;
    if ((!pool) || (size <= 0) || (!ptr))
    {
        return -EINVAL;
    }
    for (int i = 0; i < pool->num_buffers; i++)
    {
        MemoryPoolAllocation* b = &pool->buffers[i];
        if (size <= b->realsize)
        {
            if (mempool_debug) printf("mempool: suitable buffer %p with size %ld at index %d\n", b->ptr, b->realsize, i);
            void *save_ptr = b->ptr;
            // use existing allocation, move MemoryPoolAllocation from buffers to allocs
            MemoryPoolAllocation* alloc = NULL;
            err = mempool_new_alloc(pool, &alloc);
            if (err != 0)
            {
                return err;
            }
            if (mempool_debug) printf("mempool: copy buffer %d to allocation %d\n", i, pool->num_allocs-1);
            mempool_copy_alloc(b, alloc);
            b->ptr = NULL;
            b->size = 0;
            b->realsize = 0;
            // update size to newly given size
            alloc->size = size;
            // delete entry in buffers by copying the last to current index
            if (mempool_debug) printf("mempool: overwrite buffer %d with buffer %d\n", i, pool->num_buffers-1);
            MemoryPoolAllocation* last = &pool->buffers[pool->num_buffers-1];
            mempool_copy_alloc(last, b);
            last->ptr = NULL;
            last->realsize = 0;
            last->size = 0;
            pool->num_buffers--;
            if (mempool_debug) printf("mempool: now %d buffers\n", pool->num_buffers);
            // copy
            *ptr = save_ptr;
            return 0;
        }
    }
    // no buffer, alloc a fresh one
    if (mempool_debug) printf("mempool: new allocation with size %ld\n", size);
    MemoryPoolAllocation* alloc = NULL;
    err = mempool_new_alloc(pool, &alloc);
    if (err != 0)
    {
        return err;
    }
    void* p = malloc(size+1);
    if (!p)
    {
        return -ENOMEM;
    }
    if (mempool_debug) printf("mempool: init new allocation %p with size %ld\n", p, size);
    alloc->ptr = p;
    alloc->size = size;
    alloc->realsize = size;
    *ptr = p;
    return 0;
}

int mempool_realloc(MemPool_t pool, size_t size, void** ptr)
{
    int err = 0;
    if ((!pool) || (size <= 0) || (!ptr))
    {
        return -EINVAL;
    }
    if (*ptr != NULL)
    {
        for (int i = 0; i < pool->num_allocs; i++)
        {
            MemoryPoolAllocation* a = &pool->buffers[i];
            if (*ptr >= a->ptr && *ptr < a->ptr+a->realsize)
            {
                if (mempool_debug) printf("mempool: suitable allocation %p with size %ld at index %d\n", a->ptr, a->realsize, i);
                void* tmp = realloc(a->ptr, size+1);
                if (!tmp)
                {
                    return -ENOMEM;
                }
                if (mempool_debug) printf("mempool: realloc to %p with size %ld\n", tmp, size);
                a->ptr = tmp;
                a->realsize = size;
                a->size = size;
                *ptr = a->ptr;
                return 0;
            }
        }
    }
    if (mempool_debug) printf("mempool: ralloc no allocation for %p, call malloc\n", *ptr);
    return mempool_alloc(pool, size, ptr);
}

int mempool_free(MemPool_t pool, void* ptr)
{
    if ((!pool) || (!ptr))
    {
        return -EINVAL;
    }
    for (int i = 0; i < pool->num_allocs; i++)
    {
        MemoryPoolAllocation* a = &pool->allocs[i];
        if (ptr >= a->ptr && ptr < a->ptr+a->realsize)
        {
            if (mempool_debug) printf("mempool: found allocation (%p/%p) for %p at index %d\n", a->ptr, a->ptr+a->realsize, ptr, i);
            if (pool->num_buffers < pool->max_buffers)
            {
                if (mempool_debug) printf("mempool: get buffer %d\n", pool->num_buffers);
                MemoryPoolAllocation* b = &pool->buffers[pool->num_buffers];
                if (mempool_debug) printf("mempool: buffer %p with size %ld\n", a->ptr, a->realsize);
                mempool_copy_alloc(a, b);
                a->ptr = NULL;
                a->size = 0;
                a->realsize = 0;
                if (mempool_debug) printf("mempool: check buffer %p with size %ld\n", b->ptr, b->realsize);
                pool->num_buffers++;
                if (mempool_debug) printf("mempool: now %d buffers\n", pool->num_buffers);
            }
            else
            {
                if (mempool_debug) printf("mempool: buffers full %d\n", pool->num_buffers);
                mempool_free_alloc(a);
            }
            if (mempool_debug) printf("mempool: delete allocation with index %d\n", pool->num_allocs-1);
            MemoryPoolAllocation* last = &pool->allocs[pool->num_allocs-1];
            mempool_copy_alloc(last, a);
            last->ptr = NULL;
            last->size = 0;
            last->realsize = 0;
            pool->num_allocs--;
            if (mempool_debug) printf("mempool: now %d allocations\n", pool->num_allocs);
            return 0;
        }
    }
    fprintf(stderr, "No allocation entry for ptr %p in mempool\n", ptr);
    return -EEXIST;
}
