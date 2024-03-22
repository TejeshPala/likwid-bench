#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <map.h>

#ifdef WITH_BSTRING
#include <bstrlib.h>
#endif

#define declare_tests(name, type) \
int do_##name##tests(type key1, type key2, type key3) \
{ \
    Map_t m; \
    char* s = NULL; \
    int ret = init_##name##map(&m, NULL); \
    if (ret != 0) \
    { \
        return -1; \
    } \
    int hidx = add_##name##map(m, key1, (void*)"World"); \
    if (hidx < 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    int fidx = add_##name##map(m, key2, (void*)"bar"); \
    if (fidx < 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    ret = get_##name##map_by_key(m, key1, (void**)&s); \
    if (ret != 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    ret = del_##name##map(m, key1); \
    if (ret != 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    s = NULL; \
    ret = get_##name##map_by_key(m, key2, (void**)&s); \
    if (ret != 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    s = NULL; \
    ret = get_##name##map_by_idx(m, fidx, (void**)&s); \
    if (ret != 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    hidx = add_##name##map(m, key1, (void*)"World"); \
    if (hidx < 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    ret = get_##name##map_by_key(m, key1, (void**)&s); \
    if (ret != 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    int sidx = add_##name##map(m, key3, (void*)"schnapp"); \
    if (sidx < 0) \
    { \
        destroy_##name##map(m); \
        return -1; \
    } \
    destroy_##name##map(m); \
    return 0; \
}

declare_tests(s, char*)
declare_tests(i, int64_t)
declare_tests(u, uint64_t)
#ifdef WITH_BSTRING
declare_tests(b, bstring)
#endif

int main(int argc, char* argv)
{
    printf("Testing string map: ");
    int ret = do_stests("Hello", "foo", "schnipp");
    printf("%s\n", (ret < 0 ? "FAIL" : "OK"));

    printf("Testing int64 map: ");
    ret = do_itests(12, 15, 743);
    printf("%s\n", (ret < 0 ? "FAIL" : "OK"));

    printf("Testing uint64 map: ");
    ret = do_utests(12, 15, 743);
    printf("%s\n", (ret < 0 ? "FAIL" : "OK"));

#ifdef WITH_BSTRING
    struct tagbstring key1 = bsStatic("Hello");
    bstring key2 = bfromcstr("foo");
    bstring key3 = bfromcstr("schnipp");
    printf("Testing bstring map: ");
    ret = do_btests(&key1, key2, key3);
    printf("%s\n", (ret < 0 ? "FAIL" : "OK"));
    bdestroy(key2);
    bdestroy(key3);
#endif
    return 0;
}
