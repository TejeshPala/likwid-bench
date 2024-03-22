#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <map.h>



#define bench_init_map(name) \
double initialize_##name##map(Map_t* map) \
{ \
    struct timespec s, e; \
    clock_gettime(CLOCK_MONOTONIC, &s); \
    init_##name##map(map, NULL); \
    clock_gettime(CLOCK_MONOTONIC, &e); \
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9); \
}

bench_init_map(s);
bench_init_map(i);
bench_init_map(u);


#define bench_destroy_map(name) \
double finalize_##name##map(Map_t map) \
{ \
    struct timespec s, e; \
    clock_gettime(CLOCK_MONOTONIC, &s); \
    destroy_##name##map(map); \
    clock_gettime(CLOCK_MONOTONIC, &e); \
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9); \
}

bench_destroy_map(s);
bench_destroy_map(i);
bench_destroy_map(u);


double add_smap_keys(Map_t map, int count, char* * keys, char* * values) \
{ \
    struct timespec s, e; \
    clock_gettime(CLOCK_MONOTONIC, &s); \
    for (int i = 0; i < count; i++) \
    { \
        add_smap(map, keys[i], values[i]); \
    } \
    clock_gettime(CLOCK_MONOTONIC, &e); \
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9); \
}

double add_imap_keys(Map_t map, int count, int64_t * keys, char* * values) \
{ \
    struct timespec s, e; \
    clock_gettime(CLOCK_MONOTONIC, &s); \
    for (int i = 0; i < count; i++) \
    { \
        add_imap(map, (int64_t)i, values[i]); \
    } \
    clock_gettime(CLOCK_MONOTONIC, &e); \
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9); \
}

double add_umap_keys(Map_t map, int count, uint64_t * keys, char* * values) \
{ \
    struct timespec s, e; \
    clock_gettime(CLOCK_MONOTONIC, &s); \
    for (int i = 0; i < count; i++) \
    { \
        add_umap(map, (uint64_t)i, values[i]); \
    } \
    clock_gettime(CLOCK_MONOTONIC, &e); \
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9); \
}


double get_smap_keys_by_key(Map_t map, int count, char* * keys, char* * values)
{
    char* tmp;
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < count; i++)
    {
        get_smap_by_key(map, keys[i], (void**)(&tmp));
        if (tmp != values[i])
            printf("Ba!\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9);
}

double get_imap_keys_by_key(Map_t map, int count, int64_t * keys, char* * values)
{
    char* tmp;
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < count; i++)
    {
        get_imap_by_key(map, (int64_t)i, (void**)(&tmp));
        if (tmp != values[i])
            printf("Ba!\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9);
}

double get_umap_keys_by_key(Map_t map, int count, uint64_t * keys, char* * values)
{
    char* tmp;
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < count; i++)
    {
        get_imap_by_key(map, (uint64_t)i, (void**)(&tmp));
        if (tmp != values[i])
            printf("Ba!\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9);
}


double get_smap_idxs_for_keys(Map_t map, int count, char* * keys)
{
    int tmp;
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < count; i++)
    {
        get_smap_idx_for_key(map, keys[i], &tmp);
        if (tmp != i)
            printf("Ba!\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9);
}

double get_imap_idxs_for_keys(Map_t map, int count, int64_t * keys)
{
    int tmp;
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < count; i++)
    {
        get_imap_idx_for_key(map, (int64_t)i, &tmp);
        if (tmp != i)
            printf("Ba!\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9);
}

double get_umap_idxs_for_keys(Map_t map, int count, uint64_t * keys)
{
    int tmp;
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < count; i++)
    {
        get_imap_idx_for_key(map, (uint64_t)i, &tmp);
        if (tmp != i)
            printf("Ba!\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9);
}


#define get_keys_by_idx(name, keytype, valuetype) \
double get_##name##map_keys_by_idx(Map_t map, int count, keytype * keys, valuetype * values) \
{ \
    valuetype tmp; \
    struct timespec s, e; \
    clock_gettime(CLOCK_MONOTONIC, &s); \
    for (int i = 0; i < count; i++) \
    { \
        get_##name##map_by_idx(map, i, (void**)(&tmp)); \
        if (tmp != values[i]) \
            printf("Ba!\n"); \
    } \
    clock_gettime(CLOCK_MONOTONIC, &e); \
    return (e.tv_sec - s.tv_sec)+((e.tv_nsec - s.tv_nsec)*1E-9); \
}

get_keys_by_idx(s, char*, char*)
get_keys_by_idx(i, int64_t, char*)
get_keys_by_idx(u, uint64_t, char*)

int get_maxlen(int count)
{
    char buf[1025];
    int ret = snprintf(buf, 1024, "%d", count);
    return ret+1;
}


int prepare_input(int count, char*** out_array)
{
    int ret = 0;
    char** input;
    int keylen = 0;

    keylen = get_maxlen(count);
    if (keylen <= 1)
        return -1;
    
    input = malloc(count * sizeof(char*));
    if (!input)
        return -1;
    for (int i = 0; i < count; i++)
    {
        input[i] = malloc(keylen+1 * sizeof(char));
        if (!input[i])
        {
            for (int j = 0; j < i; j++)
            {
                free(input[j]);
            }
            free(input);
            return -1;
        }
        ret = snprintf(input[i], keylen+1, "%0d", i);
        input[i][ret] = '\0';
    }
    *out_array = input;
    return 0;
}

void release_input(int count, char** input)
{
    for (int i = 0; i < count; i++) free(input[i]);
    free(input);
}


int run_smap(int count)
{
    int ret = 0;
    char** input;

    prepare_input(count, &input);
    
    Map_t smap;
    printf("#count init add getbykey getbyidx getidx destroy\n");
    for (int i = 100; i <= count; i += 100) {
        printf("%d ", i);
        printf("%f ", initialize_smap(&smap));
        printf("%f ", add_smap_keys(smap, i, input, input));
        printf("%f ", get_smap_keys_by_key(smap, i, input, input));
        printf("%f ", get_smap_keys_by_idx(smap, i, input, input));

        printf("%f ", get_smap_idxs_for_keys(smap, i, input));
        printf("%f ", finalize_smap(smap));
        printf("\n");
    }

    release_input(count, input);

    return 0;
}

int run_imap(int count)
{
    int ret = 0;
    char** input;

    prepare_input(count, &input);
    
    Map_t imap;
    printf("#count init add getbykey getbyidx getidx destroy\n");
    for (int i = 100; i <= count; i += 100) {
        printf("%d ", i);
        printf("%f ", initialize_imap(&imap));
        printf("%f ", add_imap_keys(imap, i, NULL, input));
        printf("%f ", get_imap_keys_by_key(imap, i, NULL, input));
        printf("%f ", get_imap_keys_by_idx(imap, i, NULL, input));
        printf("%f ", get_imap_idxs_for_keys(imap, i, NULL));
        printf("%f ", finalize_imap(imap));
        printf("\n");
    }

    release_input(count, input);

    return 0;
}

int run_umap(int count)
{
    int ret = 0;
    char** input;

    prepare_input(count, &input);
    
    Map_t umap;
    printf("#count init add getbykey getbyidx getidx destroy\n");
    for (int i = 100; i <= count; i += 100) {
        printf("%d ", i);
        printf("%f ", initialize_umap(&umap));
        printf("%f ", add_umap_keys(umap, i, NULL, input));
        printf("%f ", get_umap_keys_by_key(umap, i, NULL, input));
        printf("%f ", get_umap_keys_by_idx(umap, i, NULL, input));
        printf("%f ", get_umap_idxs_for_keys(umap, i, NULL));
        printf("%f ", finalize_umap(umap));
        printf("\n");
    }

    release_input(count, input);

    return 0;
}

int main(int argc, char* argv[])
{
    printf("Running benchmark for string map (smap)\n");
    run_smap(20000);
    printf("\n");
    printf("Running benchmark for int64_t map (imap)\n");
    run_imap(20000);
    printf("\n");
    printf("Running benchmark for uint64_t map (umap)\n");
    run_umap(20000);
    printf("\n");
    return 0;
}


