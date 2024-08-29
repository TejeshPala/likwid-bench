#ifndef MEMPOOL_H
#define MEMPOOL_H

typedef struct {
    size_t size;
    size_t realsize;
    void* ptr;
} MemoryPoolAllocation;

typedef struct {
    int num_allocs;
    int max_allocs;
    int inc_factor;
    MemoryPoolAllocation *allocs;
    int num_buffers;
    int max_buffers;
    MemoryPoolAllocation *buffers;
    int debug;
} MemPool;
typedef MemPool* MemPool_t;

#define MEMPOOL_DEFAULT_INC_FACTOR 10
#define MEMPOOL_DEFAULT_MAX_BUFFERS 100

int mempool_init(MemPool_t* pool);
int mempool_init_opts(MemPool_t* pool, int max_buffers, int alloc_inc_factor);
void mempool_close(MemPool_t pool);
int mempool_alloc(MemPool_t pool, size_t size, void** ptr);
int mempool_realloc(MemPool_t pool, size_t size, void** ptr);
int mempool_free(MemPool_t pool, void* ptr);
void mempool_set_debug(int value);

#endif /* MEMPOOL_H */
